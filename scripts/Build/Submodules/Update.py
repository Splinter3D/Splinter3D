import subprocess
from Logger import logger
from ParseArgs import args

__all__ = ["update_submodules"]

def update_submodules():
    if args.skip_submodules == True:
        logger.info("Ignoring submodules update as per the provided argument.")
        return
    logger.info("Updating submodules...")
    try:
        subprocess.run(["git", "submodule", "update", "--init", "--recursive"], check=True)
        logger.info("Submodules updated successfully.")
    except subprocess.CalledProcessError as e:
        logger.error(f"Failed to update submodules: {e}")
        raise
