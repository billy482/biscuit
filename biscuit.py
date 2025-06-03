#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Main entry point for the Biscuit application.

This script imports the `biscuit_main` function from the `biscuit` package and executes it
with the command-line arguments. It serves as the CLI launcher for the Biscuit tool.

Usage:
	python biscuit.py [options]

Returns:
	The exit code of the Biscuit application.
"""

from biscuit import biscuit_main

if __name__ == '__main__':
	import sys
	sys.exit(biscuit_main(sys.argv[1:]))
