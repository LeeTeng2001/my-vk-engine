You might want to put SDL3.dll in the root directory

## Introduction
- Face is rendered in counter-clockwise order
- World space & camera space is up:+y, right: +x, forward:-z
- Clip space: up:-y, right: +x, forward:+z
- Front-Face: counter-clockwise
- Tex coord: top left 0,0, right +x, down +y
- Other than Vulkan SDK, every other dependencies are managed as submodules

## Code style
- https://stackoverflow.com/questions/3706379/what-is-a-good-naming-convention-for-vars-methods-etc-in-c
- https://stackoverflow.com/questions/1228161/why-use-prefixes-like-m-on-data-members-in-c-classes
- Class in PascalCase
- local vars in camelCase
- functions in camelCase
- private class member prefix with _

## Ideas
- node inheritance actors, world transform & local transform
- animation system
- lookup key actor, components, not vector

## Resources
- Quaternion: https://www.youtube.com/watch?v=xoTqBqQtrY4

