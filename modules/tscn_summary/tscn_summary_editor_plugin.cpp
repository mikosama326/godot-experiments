#include "tscn_summary_editor_plugin.h"

#include "core/config/engine.h"
#include "core/variant/dictionary.h"
#include "editor/editor_interface.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/spin_box.h"

#include "tscn_summarizer.h"
#ifdef TOOLS_ENABLED
TscnSummaryEditorPlugin::TscnSummaryEditorPlugin() {
	print_line("[tscn_summary] TscnSummaryEditorPlugin ctor");
	dock = memnew(VBoxContainer);
	dock->set_name("TSCN Summary");

	// Title
	Label *title = memnew(Label);
	title->set_text("TSCN Summarizer (Built-in)");
	dock->add_child(title);

	// Scene row
	HBoxContainer *scene_row = memnew(HBoxContainer);
	Label *sl = memnew(Label);
	sl->set_text("Scene (.tscn):");
	scene_row->add_child(sl);
	Button *sb = memnew(Button);
	sb->set_text("Choose");
	scene_row->add_child(sb);
	scene_path_le = memnew(LineEdit);
	scene_path_le->set_editable(false);
	scene_row->add_child(scene_path_le);
	dock->add_child(scene_row);

	// Out row
	HBoxContainer *out_row = memnew(HBoxContainer);
	Label *ol = memnew(Label);
	ol->set_text("Output (.json/.jsonl):");
	out_row->add_child(ol);
	Button *ob = memnew(Button);
	ob->set_text("Choose");
	out_row->add_child(ob);
	out_path_le = memnew(LineEdit);
	out_path_le->set_editable(false);
	out_row->add_child(out_path_le);
	dock->add_child(out_row);

	// Options
	cb_include_scripts = memnew(CheckBox);
	cb_include_scripts->set_text("Include scripts");
	cb_include_scripts->set_pressed(true);
	dock->add_child(cb_include_scripts);
	cb_sample_instances = memnew(CheckBox);
	cb_sample_instances->set_text("Sample a few instance positions");
	cb_sample_instances->set_pressed(true);
	dock->add_child(cb_sample_instances);
	cb_compute_stats = memnew(CheckBox);
	cb_compute_stats->set_text("Compute basic position stats");
	cb_compute_stats->set_pressed(true);
	dock->add_child(cb_compute_stats);
	cb_jsonl = memnew(CheckBox);
	cb_jsonl->set_text("Write JSONL (one archetype per line)");
	cb_jsonl->set_pressed(false);
	dock->add_child(cb_jsonl);

	HBoxContainer *sc_row = memnew(HBoxContainer);
	Label *scl = memnew(Label);
	scl->set_text("Sample count");
	sc_row->add_child(scl);
	sp_sample_count = memnew(SpinBox);
	sp_sample_count->set_min(0);
	sp_sample_count->set_max(100);
	sp_sample_count->set_step(1);
	sp_sample_count->set_value(5);
	sp_sample_count->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	sc_row->add_child(sp_sample_count);
	dock->add_child(sc_row);

	cb_follow_refs = memnew(CheckBox);
	cb_follow_refs->set_text("Follow referenced subscenes");
	cb_follow_refs->set_pressed(false);
	dock->add_child(cb_follow_refs);

	HBoxContainer *rd = memnew(HBoxContainer);
	Label *rdl = memnew(Label);
	rdl->set_text("Reference depth");
	rd->add_child(rdl);
	sp_ref_depth = memnew(SpinBox);
	sp_ref_depth->set_min(0);
	sp_ref_depth->set_max(6);
	sp_ref_depth->set_step(1);
	sp_ref_depth->set_value(1);
	sp_ref_depth->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	rd->add_child(sp_ref_depth);
	dock->add_child(rd);

	cb_include_instances = memnew(CheckBox);
	cb_include_instances->set_text("Include full instance list (positions)");
	cb_include_instances->set_pressed(false); // default off (keeps output compact)
	dock->add_child(cb_include_instances);

	// Run
	Button *run = memnew(Button);
	run->set_text("Summarize");
	run->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dock->add_child(run);

	// File dialogs
	scene_picker = memnew(FileDialog);
	scene_picker->set_access(FileDialog::ACCESS_RESOURCES);
	scene_picker->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	scene_picker->add_filter("*.tscn; TSCN Scene");
	dock->add_child(scene_picker);

	out_picker = memnew(FileDialog);
	out_picker->set_access(FileDialog::ACCESS_FILESYSTEM);
	out_picker->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
	out_picker->add_filter("*.json; JSON file");
	out_picker->add_filter("*.jsonl; JSON Lines");
	dock->add_child(out_picker);

	// Connections
	sb->connect("pressed", callable_mp(this, &TscnSummaryEditorPlugin::_on_pick_scene_pressed));
	ob->connect("pressed", callable_mp(this, &TscnSummaryEditorPlugin::_on_pick_out_pressed));
	scene_picker->connect("file_selected", callable_mp(this, &TscnSummaryEditorPlugin::_on_scene_chosen));
	out_picker->connect("file_selected", callable_mp(this, &TscnSummaryEditorPlugin::_on_out_chosen));
	run->connect("pressed", callable_mp(this, &TscnSummaryEditorPlugin::_on_run_pressed));

	add_control_to_dock(DOCK_SLOT_RIGHT_UL, dock);
}

void TscnSummaryEditorPlugin::_on_pick_scene_pressed() {
	scene_picker->popup_centered_ratio();
}
void TscnSummaryEditorPlugin::_on_pick_out_pressed() {
	out_picker->popup_centered_ratio();
}
void TscnSummaryEditorPlugin::_on_scene_chosen(const String &p_path) {
	selected_scene_path = p_path;
	scene_path_le->set_text(p_path);
}
void TscnSummaryEditorPlugin::_on_out_chosen(const String &p_path) {
	selected_out_path = p_path;
	out_path_le->set_text(p_path);
}

void TscnSummaryEditorPlugin::_on_run_pressed() {
	if (selected_scene_path.is_empty() || selected_out_path.is_empty()) {
		print_error("Select a .tscn and an output path first.");
		return;
	}
	Dictionary options;
	options["include_scripts"] = cb_include_scripts->is_pressed();
	options["sample_instances"] = cb_sample_instances->is_pressed();
	options["compute_stats"] = cb_compute_stats->is_pressed();
	options["jsonl_chunks"] = cb_jsonl->is_pressed();
	options["sample_count"] = (int)sp_sample_count->get_value();
	options["include_instances"] = cb_include_instances->is_pressed();

	// New: follow references
	Vector<String> to_process;
	to_process.push_back(selected_scene_path);

	if (cb_follow_refs->is_pressed()) {
		const int max_depth = (int)sp_ref_depth->get_value();
		Array refs = TSCNSummarizer::find_referenced_scenes(selected_scene_path, max_depth);
		for (int i = 0; i < refs.size(); i++) {
			String rp = refs[i];
			// Avoid duplicates and the root scene path
			if (rp != selected_scene_path) {
				to_process.push_back(rp);
			}
		}
	}

	// Summarize the root first, then referenced scenes.
	// You can either write separate files or aggregate; here we write separate files
	// next to the chosen path with a suffix.
	bool all_ok = true;

	// 1) Write the main one to exactly selected_out_path
	Dictionary result_main = TSCNSummarizer::summarize(selected_scene_path, options);
	if (result_main.is_empty() || !TSCNSummarizer::write_output(result_main, selected_out_path, options)) {
		print_error("Failed to write main summary.");
		all_ok = false;
	}

	// 2) For referenced scenes, write sibling files with a suffix
	const String base_dir = selected_out_path.get_base_dir();
	const String base_file = selected_out_path.get_file().get_basename(); // without .json/.jsonl
	const String ext = selected_out_path.get_extension(); // "json" or "jsonl"

	for (int i = 1; i < to_process.size(); i++) {
		const String scene_p = to_process[i];
		Dictionary r = TSCNSummarizer::summarize(scene_p, options);
		if (r.is_empty()) {
			all_ok = false;
			continue;
		}

		// Make a safe filename from the scene path
		String safe = scene_p;
		safe = safe.replace("res://", "");
		safe = safe.replace("/", "_").replace("\\", "_");

		const String out_p = vformat("%s/%s__%s.%s", base_dir, base_file, safe, ext);
		if (!TSCNSummarizer::write_output(r, out_p, options)) {
			print_error(vformat("Failed to write summary for: %s", scene_p));
			all_ok = false;
		}
	}

	if (all_ok) {
		print_line("Summaries written (root + referenced).");
	}
}
#endif // #ifdef TOOLS_ENABLED
