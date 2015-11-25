#!/bin/bash
autoheader
autoconf
aclocal
automake --add-missing
autoconf

