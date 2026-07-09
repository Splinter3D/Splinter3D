import shutil
from Logger import logger
from Platform import get_platform, PLATFORM_REQUIRED_TOOLS

__all__ = ["check_requirements"]

def check_requirements():
    logger.info("Checking requirements...")
    platform = get_platform()
    required_tools = PLATFORM_REQUIRED_TOOLS[platform]
    for tool_set in required_tools:
        if not any(shutil.which(tool) for tool in tool_set):
            logger.error(f"Missing required tools: {' or '.join(tool_set)}")
            raise RuntimeError(f"Missing required tools: {' or '.join(tool_set)}")
    logger.info("All requirements satisfied.")
