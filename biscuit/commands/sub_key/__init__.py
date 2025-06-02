# -*- coding: utf-8 -*-

from .generate import generate_parse
from .import_key import import_parse
from .list import list_parse

parsers = [generate_parse, import_parse, list_parse]

__all__ = ["parsers"]
