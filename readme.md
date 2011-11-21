# Introduction

This is a half-completed Android port of Fabien Sanglard's iOS game, [Shmup](http://fabiensanglard.net/shmup_source_code/index.php). Contained within this project are all the necessary Android files, as well as necessary modifications made to Fabien's original iOS C files.

# Changes

Probably the biggest change here is that the Android port depends on PNG files, while the iOS one happily uses compressed PVRTC files. Unfortunately, not all Android devices support texture compression, so the game relies on the full PNG assets.

# What's Not Working?

Basically, the touchscreen. There's little to no documentation online as to how to use the touchscreen in pure C applications. Perhaps this will change in the future.

# License

This game is licensed under GPL:

    SHMUP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SHMUP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SHMUP.  If not, see <http://www.gnu.org/licenses/>.

Note that absolutely under **no conditions** are any of the game data resources&mdash;such as graphics, music, sound effects, and so on&mdash;to be redistributed. They are **not** licensed under GPL!