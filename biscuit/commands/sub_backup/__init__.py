# -*- coding: utf-8 -*-

from biscuit.commands import parsers
from .list import list_parse

parsers = [list_parse]

__all__ = ["parsers"]
