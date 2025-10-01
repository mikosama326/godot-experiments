#pragma once
#include "editor/plugins/editor_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/spin_box.h"
#ifdef TOOLS_ENABLED
class TscnSummaryEditorPlugin : public EditorPlugin {
	GDCLASS(TscnSummaryEditorPlugin, EditorPlugin);

	VBoxContainer *dock = nullptr;
	FileDialog *scene_picker = nullptr;
	FileDialog *out_picker = nullptr;
	LineEdit *scene_path_le = nullptr;
	LineEdit *out_path_le = nullptr;

	CheckBox *cb_include_scripts = nullptr;
	CheckBox *cb_sample_instances = nullptr;
	CheckBox *cb_compute_stats = nullptr;
	CheckBox *cb_jsonl = nullptr;
	SpinBox *sp_sample_count = nullptr;
	CheckBox *cb_follow_refs = nullptr;
	SpinBox *sp_ref_depth = nullptr;

	String selected_scene_path;
	String selected_out_path;

protected:
	static void _bind_methods() {}

public:
	TscnSummaryEditorPlugin();
	~TscnSummaryEditorPlugin() override {}

	String get_plugin_name() const override { return "TSCN Summary"; }
	virtual bool has_main_screen() const override { return false; }

	void _on_pick_scene_pressed();
	void _on_pick_out_pressed();
	void _on_scene_chosen(const String &p_path);
	void _on_out_chosen(const String &p_path);
	void _on_run_pressed();
};
#endif //#ifdef TOOLS_ENABLED
