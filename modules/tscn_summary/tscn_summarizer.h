#pragma once
#include "core/object/object.h"
#include "core/string/ustring.h"
#include "core/variant/dictionary.h"

class TSCNSummarizer : public Object {
	GDCLASS(TSCNSummarizer, Object);

protected:
	static void _bind_methods();

public:
	static Dictionary summarize(const String &scene_path, const Dictionary &options);
	static bool write_output(const Dictionary &result, const String &out_path, const Dictionary &options);

	static Array find_referenced_scenes(const String &scene_path, int max_depth = 1);
	static Array summarize_batch(const Array &scene_paths, const Dictionary &options); // optional helper
};
