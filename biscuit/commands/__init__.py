from .backup import backup_parse
from .config import config_parse

parsers = [backup_parse, config_parse]

__all__ = [parsers]
