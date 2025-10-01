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

	Dictionary result = TSCNSummarizer::summarize(selected_scene_path, options);
	if (result.is_empty()) {
		print_error("Summarizer returned empty result.");
		return;
	}
	bool ok = TSCNSummarizer::write_output(result, selected_out_path, options);
	if (!ok) {
		print_error("Failed to write output.");
	} else {
		print_line(String("Wrote summary to: ") + selected_out_path);
	}
}
#endif // #ifdef TOOLS_ENABLED
