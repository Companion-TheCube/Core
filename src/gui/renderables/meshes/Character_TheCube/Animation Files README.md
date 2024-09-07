The animations are defined in json files prepended with "anim_" or "animation_". End file contains a single object with the following properties:

- "name": string, the name of the animation
- "expression": string, the expression to activate with the animation
- "frames": array of frame objects

Each frame object contains the following properties:

- "time": {"start": number, "end": number}, the time range of the frame in frames. The start time is inclusive, the end time is exclusive.
- "type": string, the type of the frame. Can be "ROTATE", "ROTATE_ABOUT", "TRANSLATE", "SCALE_XYZ", "UNIFORM_SCALE", or "RETURN_HOME".
- "value": number, the value used to calculate the transformation. The meaning of this value depends on the type of the frame.
- "axis": {"x": number, "y": number, "z": number}, the axis of rotation or translation.
- "point": {"x": number, "y": number, "z": number}, the point about which to rotate.
- "easing": string, the easing function to use for the frame. Can be "linear", "easeIn", "easeOut", or "easeInOut".

For "ROTATE" frames, the value is the angle in degrees to rotate about the axis. The character will be about its origin.
For "ROTATE_ABOUT" frames, the value is the angle in degrees to rotate about the point. The axis is the axis of rotation.
For "TRANSLATE" frames, the value is the distance to translate along the axis.
For "SCALE_XYZ" frames, the value is the scale factor along the axis.
For "UNIFORM_SCALE" frames, the value is the scale factor.
For "RETURN_HOME" frames, the value must be 1. The character will return to its home position.

Start times do not have to be in order and they can overlap. The character will interpolate between frames as necessary. The character will always start at the home position and end at the home position. If a RETURN_HOME frame is not present, the character will jump to the home position at the end of the animation. The home position is defined in the character.json file.