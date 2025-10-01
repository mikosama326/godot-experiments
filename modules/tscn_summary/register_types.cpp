#include "register_types.h"
#include "tscn_summarizer.h"

#ifdef TOOLS_ENABLED
#include "editor/plugins/editor_plugin.h"
#include "tscn_summary_editor_plugin.h"
#endif
#include <modules/register_module_types.h>

void initialize_tscn_summary_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		ClassDB::register_class<TSCNSummarizer>();
	}
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		EditorPlugins::add_by_type<TscnSummaryEditorPlugin>();
	}
#endif
}

void uninitialize_tscn_summary_module(ModuleInitializationLevel p_level) {
	(void)p_level;
}
