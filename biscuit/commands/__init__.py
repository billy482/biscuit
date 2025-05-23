# -*- coding: utf-8 -*-

from .backup import backup_parse
from .config import config_parse
from .key import key_parse

parsers = [backup_parse, config_parse, key_parse]

__all__ = ["parsers"]
