import os
import shutil

def before_build(source, target, env):
    secrets_path = os.path.join(env['PROJECTSRC_DIR'], "secrets.h")
    example_path = os.path.join(env['PROJECTSRC_DIR'], "secrets_example.h")

    if not os.path.exists(secrets_path):
        print("⚠️ secrets.h manquant, création depuis secrets_example.h")
        shutil.copy(example_path, secrets_path)
