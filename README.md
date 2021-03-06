# vvd, Universal virtual disk management tool

⚠ **EARLY DEVELOPMENT: MAY EXPLODE VIOLENTLY** ⚠

⚠ **DANGEROUS: MISSING UNIT TESTS** ⚠

⚠ **NOT PRODUCTION READY** ⚠

vvd aims to be a simple, robust, stable, universal tool to administrate
virtual disk images.

User manual: `docs/vvd.1`

Technical Manual: Coming soon

# Introduction

vvd is an attempt to be a stand-alone and all-in-one tool to manage virtual
disk images.

Personal goals:
- [ ] Finally compact my vmdk Windows disk images.

# Compiling

## Using the build scripts

There are two build scripts (`m.cmd` on Windows, and `m` on Posix platforms)
that use clang-cl (Windows) or clang (Posix) by default.

You may set (and export) the `CC` (C compiler) and `CF` (C flags) variables
for the scripts. For more info, you can invoke the scripts with `--help`.

## Using tup

There is experimental tup support. This only builds the object files since
tup will complain if the compiler creates a temporary file not within tup's
database. You can still invoke `m link` to complete the process.

# Homes

- https://github.com/dd86k/vvd
- https://git.dd86k.space/dd86k/vvd

# DISCLAIMER

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

For more information, see LICENSE.