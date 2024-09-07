The character.json file define all the necessary information to load a character. It contains a single object with the following properties:

- "name": string, the name of the character
- "objects": array of strings that are the names of the objects that make up the character. These must match the filenames (minus the extension) of the object files.
- "animations": array of strings that are the names of the animation files. These must match the filenames (including the extension) of the animation json files.
- "expressions": array of strings that are the names of the expression files. These must match the filenames (including the extension) of the expression json files.
- "initPos": {"x": number, "y": number, "z": number}, the initial position of the character
- "initRot": {"x": number, "y": number, "z": number, "value": number}, the initial rotation of the character. The value is the angle in degrees to rotate about the axis.
- "initScale": {"x": number, "y": number, "z": number}, the initial scale of the character