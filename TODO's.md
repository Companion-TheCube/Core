# TODO's
- Any file that has user-facing strings should have Gettext support using #define _(string) gettext(string). We also need to add the necessary code to the CMakeLists.txt file to generate the .pot file and the .mo files. Probably should keep track of which files have strings that need to be translated.
- I need to make a github for the translations so that people can contribute translations.
- Add API endpoints to Logger for logging from outside apps
- Apps that provide characters (for the UI) will have to provide a blob of the character data that we can deserialize into the character object. 
- Add status bar to the GUI
- Make the popup messageBox more like a dialog box 
- Write tests
- Migrate from SFML to GLFW