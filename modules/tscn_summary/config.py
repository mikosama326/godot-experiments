# Required: tell SCons this module can build on this platform.
def can_build(env, platform):
    # You can check env['tools'] here if you only want editor builds.
    return True

# Optional: tweak the build environment for this module.
def configure(env):
    # Example: add preprocessor defines or extra include paths if needed.
    # env.Prepend(CPPDEFINES=['TSCN_SUMMARY_ENABLED'])
    pass

# Optional: enable/disable the module (useful if you add build options).
def is_enabled():
    return True

# Optional: class docs integration (you can skip if you have no docs).
def get_doc_classes():
    # Expose classes for the doc generator if you later add XML docs.
    return ['TSCNSummarizer']

def get_doc_path():
    # Path (relative to this folder) containing your doc XMLs, if any.
    # Create modules/tscn_summary/doc/ later, or return an empty string.
    return 'doc'

# Debug: prove SCons loaded this file.
print('>>> config.py: tscn_summary loaded')