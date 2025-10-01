#include "tscn_summarizer.h"


#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/templates/hash_map.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"


#include "scene/2d/node_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/2d/animated_sprite_2d.h"


#include "scene/3d/node_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include <scene/3d/physics/collision_shape_3d.h>
#include <scene/2d/physics/collision_shape_2d.h>

// Helper: get an identifying resource path for common node types.
static String _primary_resource_path(Object *n) {
	if (auto mi = Object::cast_to<MeshInstance3D>(n)) {
		if (mi->get_mesh().is_valid() && mi->get_mesh()->get_path() != String()) {
			return mi->get_mesh()->get_path();
		}
	}
	if (auto mmi = Object::cast_to<MultiMeshInstance3D>(n)) {
		if (mmi->get_multimesh().is_valid() && mmi->get_multimesh()->get_path() != String()) {
			return mmi->get_multimesh()->get_path();
		}
	}
	if (auto s2 = Object::cast_to<Sprite2D>(n)) {
		if (s2->get_texture().is_valid() && s2->get_texture()->get_path() != String()) {
			return s2->get_texture()->get_path();
		}
	}
	if (auto as2 = Object::cast_to<AnimatedSprite2D>(n)) {
		if (as2->get_sprite_frames().is_valid() && as2->get_sprite_frames()->get_path() != String()) {
			return as2->get_sprite_frames()->get_path();
		}
	}
	if (auto cs3 = Object::cast_to<CollisionShape3D>(n)) {
		if (cs3->get_shape().is_valid()) {
			return cs3->get_shape()->get_class();
		}
	}
	if (auto cs2 = Object::cast_to<CollisionShape2D>(n)) {
		if (cs2->get_shape().is_valid()) {
			return cs2->get_shape()->get_class();
		}
	}
	return String();
}

// Helper: extract global position as Dictionary{x,y,z} when applicable.
static Variant _extract_position(Object *n) {
	if (auto n3 = Object::cast_to<Node3D>(n)) {
		const Vector3 p = n3->get_global_transform().origin;
		Dictionary d;
		d["x"] = p.x;
		d["y"] = p.y;
		d["z"] = p.z;
		return d;
	}
	if (auto n2 = Object::cast_to<Node2D>(n)) {
		const Vector2 p = n2->get_global_position();
		Dictionary d;
		d["x"] = p.x;
		d["y"] = p.y;
		d["z"] = 0.0;
		return d;
	}
	return Variant();
}

static void _accumulate_axis(Dictionary &axis, double v) {
	if (axis["min"].get_type() == Variant::NIL || v < double(axis["min"])) {
		axis["min"] = v;
	}
	if (axis["max"].get_type() == Variant::NIL || v > double(axis["max"])) {
		axis["max"] = v;
	}
	double n = axis.has("n") ? double(axis["n"]) : 0.0;
	double mean = axis.has("mean") ? double(axis["mean"]) : 0.0;
	n += 1.0;
	mean += (v - mean) / n;
	axis["n"] = n;
	axis["mean"] = mean;
}

void TSCNSummarizer::_bind_methods() {
	ClassDB::bind_static_method("TSCNSummarizer", D_METHOD("summarize", "scene_path", "options"), &TSCNSummarizer::summarize);
	ClassDB::bind_static_method("TSCNSummarizer", D_METHOD("write_output", "result", "out_path", "options"), &TSCNSummarizer::write_output);
}

Dictionary TSCNSummarizer::summarize(const String &scene_path, const Dictionary &options) {
	// ---- options ----
	const bool include_scripts = options.get("include_scripts", true);
	const bool sample_instances = options.get("sample_instances", true);
	const bool compute_stats = options.get("compute_stats", true);
	const int sample_count = int(options.get("sample_count", 5));

	Dictionary result;

	// ---- load & instantiate safely for editor ----
	Ref<PackedScene> packed = ResourceLoader::load(scene_path);
	ERR_FAIL_COND_V_MSG(packed.is_null(), result, vformat("Failed to load scene: %s", scene_path));

	Node *root = Object::cast_to<Node>(packed->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE));
	ERR_FAIL_COND_V_MSG(!root, result, vformat("Failed to instantiate scene: %s", scene_path));

	// ---- traversal state (typed, no boxing) ----
	Vector<Node *> nodes;
	Vector<String> paths;
	Vector<int> depths;
	nodes.push_back(root);
	paths.push_back(String(root->get_name()));
	depths.push_back(0);

	// ---- aggregates ----
	Dictionary archetype_map; // key -> {type, resource, script, count, samples:Array, stats:Dict}
	int node_count = 0;
	int depth_max = 0;
	double branching_sum = 0.0;
	int branching_cnt = 0;

	// ---- walk ----
	while (!nodes.is_empty()) {
		Node *n = nodes[0];
		nodes.remove_at(0);
		String p = paths[0];
		paths.remove_at(0);
		int dz = depths[0];
		depths.remove_at(0);
		if (!n) {
			continue;
		}

		node_count++;
		depth_max = MAX(depth_max, dz);
		branching_sum += n->get_child_count();
		branching_cnt++;

		// archetype key parts
		const String type = n->get_class();
		const String resource_path = _primary_resource_path(n);

		String script_path;
		if (include_scripts) {
			Ref<Script> s = n->get_script();
			if (s.is_valid()) {
				const String sp = s->get_path();
				if (!sp.is_empty()) {
					script_path = sp;
				}
			}
		}

		const String key = vformat("%s|%s|%s", type, resource_path, script_path);

		// fetch/create record
		Dictionary rec = archetype_map.has(key) ? Dictionary(archetype_map[key]) : Dictionary();
		if (rec.is_empty()) {
			Dictionary stats;
			Dictionary ax;
			ax["min"] = Variant();
			ax["max"] = Variant();
			ax["mean"] = 0.0;
			ax["n"] = 0.0;
			stats["x"] = ax.duplicate();
			stats["y"] = ax.duplicate();
			stats["z"] = ax.duplicate();

			rec["type"] = type;
			rec["resource"] = resource_path;
			rec["script"] = script_path;
			rec["count"] = 0;
			rec["samples"] = Array();
			rec["stats"] = stats;
		}
		rec["count"] = int(rec["count"]) + 1;

		// sample & stats
		const Variant pos_v = _extract_position(n);
		if (pos_v.get_type() == Variant::DICTIONARY) {
			const Dictionary pos = pos_v;

			if (sample_instances) {
				Array samples = rec["samples"];
				if (samples.size() < sample_count) {
					Dictionary s;
					s["node_path"] = p;
					Array arr;
					arr.push_back(pos["x"]);
					arr.push_back(pos["y"]);
					arr.push_back(pos["z"]);
					s["position"] = arr;
					samples.push_back(s);
					rec["samples"] = samples;
				}
			}

			if (compute_stats) {
				Dictionary stats = rec["stats"];
				Dictionary xs = stats["x"];
				_accumulate_axis(xs, double(pos["x"]));
				stats["x"] = xs;
				Dictionary ys = stats["y"];
				_accumulate_axis(ys, double(pos["y"]));
				stats["y"] = ys;
				Dictionary zs = stats["z"];
				_accumulate_axis(zs, double(pos["z"]));
				stats["z"] = zs;
				rec["stats"] = stats;
			}
		}

		archetype_map[key] = rec;

		// queue children
		const int cc = n->get_child_count();
		for (int i = 0; i < cc; i++) {
			Node *c = n->get_child(i);
			if (!c) {
				continue;
			}
			nodes.push_back(c);
			paths.push_back(p + "/" + c->get_name());
			depths.push_back(dz + 1);
		}
	}

	// ---- build output ----
	Array archetypes_arr;
	Array keys = archetype_map.keys();
	archetypes_arr.resize(keys.size());
	for (int i = 0; i < keys.size(); i++) {
		const String k = keys[i];
		const Dictionary rec = archetype_map[k];

		Dictionary out;
		out["key"] = k;
		out["type"] = rec["type"];
		out["resource"] = rec["resource"];
		out["script"] = rec["script"];
		out["count"] = rec["count"];
		out["samples"] = rec["samples"];
		out["position_stats"] = rec["stats"];
		archetypes_arr[i] = out;
	}

	// (Optional) simple sort; for strict count-desc, we can add a custom comparator later.
	archetypes_arr.sort();

	Dictionary engine;
	engine["name"] = "godot";
	{
		Dictionary vi = Engine::get_singleton()->get_version_info();
		engine["version"] = vi.get("string", String()); // supply default to satisfy IntelliSense
	}

	Dictionary topo;
	topo["max_depth"] = depth_max;
	topo["mean_branching_factor"] = (branching_cnt > 0) ? (branching_sum / double(branching_cnt)) : 0.0;

	result["scene_path"] = scene_path;
	result["engine"] = engine;
	result["node_count"] = node_count;
	result["unique_archetypes"] = archetypes_arr.size();
	result["topology"] = topo;
	result["archetypes"] = archetypes_arr;

	// cleanup
	root->queue_free();
	return result;
}


bool TSCNSummarizer::write_output(const Dictionary &result, const String &out_path, const Dictionary &options) {
	const bool jsonl = options.get("jsonl_chunks", false);

	const String dir = out_path.get_base_dir();
	if (!dir.is_empty() && !DirAccess::dir_exists_absolute(dir)) {
		DirAccess::make_dir_recursive_absolute(dir);
	}

	Ref<FileAccess> f = FileAccess::open(out_path, FileAccess::WRITE);
	if (f.is_null()) {
		return false;
	}

	if (jsonl) {
		Dictionary header;
		header["scene_path"] = result["scene_path"];
		header["engine"] = result["engine"];
		header["node_count"] = result["node_count"];
		header["unique_archetypes"] = result["unique_archetypes"];
		header["topology"] = result["topology"];
		header["record_type"] = "header";
		f->store_line(JSON::stringify(header));
		Array arr = result["archetypes"];
		for (int i = 0; i < arr.size(); ++i) {
			Dictionary a = arr[i];
			a["record_type"] = "archetype";
			f->store_line(JSON::stringify(a));
		}
	} else {
		f->store_string(JSON::stringify(result, "\t"));
	}

	return true;
}
