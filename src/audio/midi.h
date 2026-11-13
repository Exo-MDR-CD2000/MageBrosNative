// Standard MIDI File sequencer replacing the MCI "sequencer" device used by the original.
// Output goes either to the platform's MIDI synthesizer (Windows' MIDI mapper, macOS' DLS
// synth) or to a built-in SoundFont synthesizer.
#pragma once
#include <string>
#include <vector>

namespace Midi {

// Where SoundFonts (.sf2) are looked for, in order: next to the program, next to MageBros.exe,
// then a "soundfonts" folder (any capitalisation) in each. `dir` ends in a separator; `name` is
// what [audio] soundfont stores for a file there: relative to the program for its own folders
// ("", "soundfonts/"), so the setting stays valid if the folder moves; otherwise the full path.
struct SoundFontDir {
    std::string dir, name;
};
std::vector<SoundFontDir> SoundFontDirs();

// Chooses the synthesizer: "" = automatic (soundfont.sf2/magebros.sf2 first, then the system
// synth, then the bundled TimGM6mb bank), "system" = the platform synthesizer only, "builtin" =
// the bundled TimGM6mb bank only, anything else = that .sf2 file only.
bool Init(const std::string& soundFontPath);
void Shutdown();
// Switch synthesizer while running (options menu); the current song keeps playing.
// The values are the same as Init's.
bool SwitchSynth(const std::string& soundFontPath);

// The bundled TimGM6mb bank, or "" when it is not installed. Used by the options menu to offer
// the built-in choice and to keep the file out of the generic .sf2 list.
std::string BuiltinSoundFontPath();

// MCI-equivalent commands
bool Open(const std::string& path);   // "close all" + "open <path> type sequencer"
void PlayFromStart();                 // "play MUSIC from 0 notify"
void Pause();                         // "stop MUSIC"
void Resume();                        // "play MUSIC notify"
void Close();                         // "close all"
bool IsLoaded();                      // a song is open (between Open and Close)

// Returns true once each time playback reached the end (MM_MCINOTIFY / MCI_NOTIFY_SUCCESSFUL)
bool PollFinished();

void SetVolume(float v);  // 0..1
const char* BackendName();

}  // namespace Midi
