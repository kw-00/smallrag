# LICENSE:
# The Glasgow Haskell Compiler License
# 
# Copyright 2002, The University Court of the University of Glasgow. 
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# - Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#  
# - Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#  
# - Neither name of the University nor the names of its contributors may be
# used to endorse or promote products derived from this software without
# specific prior written permission. 
# 
# THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY COURT OF THE UNIVERSITY OF
# GLASGOW AND THE CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# UNIVERSITY COURT OF THE UNIVERSITY OF GLASGOW OR THE CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

#!/bin/sh

# POSIX shell implementation of `realpath --relative-to=$1 $2.
# This is an adaptation of the implementation from
# <https://github.com/Offirmo/offirmo-shell-lib>.

# returns relative path to $2=$target from $1=$source
## NOTE : path are compared in text only. They don’t have to exist
##        and they WONT be normalized/escaped
## Result in "$return_value"# both $1 and $2 are absolute paths beginning with /

src="$1"
target="$2"

common_part="$src"
result=""

while test "${target#"$common_part"}" = "${target}" ; do
    #echo "common_part is now : \"$common_part\""
    #echo "result is now      : \"$result\""
    #echo "target#common_part : \"${target#$common_part}\""
    # no match, means that candidate common part is not correct
    # go up one level (reduce common part)
    common_part="$(dirname "$common_part")"
    # and record that we went back
    if test -z "$result" ; then
        result=".."
    else
        result="../$result"
    fi
done

#echo "common_part is     : \"$common_part\""

if test "$common_part" = "/" ; then
    # special case for root (no common path)
    result="$result/"
fi

# since we now have identified the common part,
# compute the non-common part
forward_part="${target#"$common_part"}"
#echo "forward_part = \"$forward_part\""

if test -n "$result" && test -n "$forward_part" ; then
    #echo "(simple concat)"
    result="$result$forward_part"
elif test -n "$forward_part" ; then
    #echo "(concat with slash removal)"
    result="$(printf "%s" "$forward_part" | cut -c 1-)"
fi

printf "%s" "$result"

