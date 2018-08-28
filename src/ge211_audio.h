#pragma once

#include "ge211_forward.h"
#include "ge211_time.h"
#include "ge211_util.h"

#include <memory>
#include <vector>

namespace ge211 {

/// Audio facilities, for playing music and sound effects.
///
/// All audio facilities are accessed via the Mixer, which is accessed
/// via the Abstract_game::get_mixer() const member function of
/// Abstract_game. If the Mixer is present (and it may not be), then it
/// can be used to load audio files as Music_track@s and Sound_effect@s.
/// The former is for playing continuous background music, whereas the
/// latter is for adding sound effects. See the Mixer documentation for
/// more.
namespace audio {

/// A music track, which can be attached to the Mixer and played.
///
/// The constructor for this class is private, and consequently it cannot be
/// constructed directly; instead, one should call the
/// Mixer::load_music(const std::string&) member function of the Mixer, which
/// returns a Music_track.
///
/// Note that Music_track has few public member functions. However, a
/// music track can be passed to these Mixer member functions to play it:
///
///  - Mixer::play_music(const Music_track&, Duration)
///  - Mixer::attach_music(const Music_track&)
///
/// Note also that the mixer can only play one music track at a time.
class Music_track
{
public:
    /// Constructs the empty music track.
    Music_track() { }

    /// Checks for the empty music track.
    bool empty() const;

    /// Checks for a non-empty music track.
    operator bool() const;

private:
    // Friends
    friend ge211::audio::Mixer;

    // Private constructor
    Music_track(const std::string& filename, detail::File_resource&&);

    // Private static factory
    static Mix_Music* load_(const std::string& filename,
                            detail::File_resource&&);

    std::shared_ptr<Mix_Music> ptr_;
};

/// A sound effect track, which can be attached to a Mixer channel and played.
///
/// The constructor for this class is private, and consequently it cannot be
/// constructed directly; instead, one should call the
/// Mixer::load_effect(const std::string&) member function of the Mixer, which
/// returns a Sound_effect.
///
/// Note that Sound_effect has few public member functions. However, an
/// effect track can be passed to the Mixer member function
/// Mixer::play_effect(const Sound_effect&, Duration)
/// to play it.
class Sound_effect
{
public:
    /// Constructs the empty sound effect track.
    Sound_effect() { }

    /// Checks for the empty sound effect track.
    bool empty() const;

    /// Checks for a non-empty sound effect track.
    operator bool() const;

    /// Returns the sound effect's volume as a number from 0.0 to 1.0.
    /// Initially this will be 1.0, but you can lower it with
    /// Sound_effect::set_volume(double).
    double get_volume() const;

    /// Sets the sound effects volume as a number from 0.0 to 1.0.
    void set_volume(double unit_value);

private:
    // Friends
    friend ge211::audio::Mixer;

    // Private constructor
    Sound_effect(const std::string& filename, detail::File_resource&&);

    // Private static factory
    static Mix_Chunk* load_(const std::string& filename,
                            detail::File_resource&&);

    std::shared_ptr<Mix_Chunk> ptr_;
};

/// The entity that coordinates playing all audio tracks.
class Mixer
{
public:
    /// The state of an audio channel.
    enum class State
    {
        /// No track is attached to the channel, or no channel is attached to
        /// the handle.
        detached,
        /// Actively playing.
        playing,
        /// In the process of fading out from playing to paused (for music) or
        /// to halted and detached (for sound effects).
        fading_out,
        /// Attached but not playing.
        paused,
    };

    /// \name Playing music
    ///@{

    /// Loads a new music track.
    Music_track load_music(const std::string& filename);

    /// Attaches the given music track to this mixer and starts it playing.
    void play_music(const Music_track&, Duration fade_in = 0.0);

    /// Attaches the given music track to this mixer. Give empty Music_track to
    /// detach the current track, if any.
    ///
    /// **PRECONDITIONS**: It is an error to attach music when other music is
    /// playing or fading out.
    void attach_music(const Music_track&);

    /// Plays the currently attached music from the current saved position,
    /// fading in if requested.
    void unpause_music(Duration fade_in = 0.0);
    /// Pauses the currently attached music, fading out if requested.
    void pause_music(Duration fade_out = 0.0);
    /// Rewinds the music to the beginning. This is only valid when the music
    /// is paused.
    void rewind_music();

    /// Gets the Music_track currently attached to this Mixer, if any.
    const Music_track& get_music() const
    {
        return current_music_;
    }

    /// Returns the current state of the attached music, if any.
    State get_music_state() const
    {
        return music_state_;
    }

    ///@}

    /// \name Playing sound effects
    ///@{

    /// Loads a new sound effect track.
    Sound_effect load_effect(const std::string& filename);

    /// How many effect channels are current unattached?
    int available_effect_channels() const;

    /// Attaches the given effect track to a channel of this mixer, starting
    /// the effect playing and returning the channel.
    Sound_effect_handle
    play_effect(const Sound_effect&, Duration fade_in = 0.0);

    /// Pauses all currently-playing effects.
    void pause_all_effects();
    /// Unpauses all currently-paused effects.
    void unpause_all_effects();

    ///@}

    ///\name Constructors, assignment operators, and destructor
    ///@{

    /// The mixer cannot be copied.
    Mixer(const Mixer&) = delete;
    /// The mixer cannot be moved.
    Mixer(const Mixer&&) = delete;
    /// The mixer cannot be copied.
    Mixer& operator=(const Mixer&) = delete;
    /// The mixer cannot be moved.
    Mixer& operator=(const Mixer&&) = delete;

    /// Destructor, to clean up the mixer's resources.
    ~Mixer();

    ///@}

private:
    // Only an Abstract_game is allowed to create a mixer. (And if there is
    // more than one Abstract_game at a time, we are in trouble.)
    friend audio::Sound_effect_handle;
    friend ge211::Abstract_game;
    friend ge211::detail::Engine;

    /// Opens the mixer, if possible, returning nullptr for failure.
    static std::unique_ptr<Mixer> open_mixer();

    /// Private constructor -- should not be called.
    Mixer();

    /// Updates the state of the channels.
    void poll_channels_();

    /// Returns the index of an empty channel, or throws if all are full.
    int find_empty_channel_() const;

    /// Registers an effect with a channel.
    Sound_effect_handle
    register_effect_(int channel, const Sound_effect& effect);
    /// Unregisters the effect associated with a channel.
    void unregister_effect_(int channel);

private:
    Music_track current_music_;
    State music_state_{State::detached};
    Pausable_timer music_position_{true};

    std::vector<Sound_effect_handle> channels_;
    int available_effect_channels_;
};

/// Used to control a Sound_effect after it is started playing on a Mixer.
class Sound_effect_handle
{
public:
    /// Constructs the empty sound effect handle, which is not actually
    /// controlling a Sound_effect.
    Sound_effect_handle() {}

    /// Checks for the empty sound effect handle.
    bool empty() const;

    /// Checks for a non-empty sound effect handle.
    operator bool() const;

    /// Pauses the effect.
    void pause();
    /// Unpauses the effect.
    void unpause();
    /// Stops the effect from playing, and unregisters it when finished.
    void stop(Duration fade_out = 0.0);

    /// Gets the Sound_effect being played by this handle.
    const Sound_effect& get_effect() const;

    /// Gets the state of this effect.
    Mixer::State get_state() const;

private:
    friend audio::Mixer;

    struct Impl_
    {
        Impl_(Mixer& m, const Sound_effect& e, int c)
                : mixer(m), effect(e), channel(c),
                  state(Mixer::State::playing) {}

        Mixer& mixer;
        Sound_effect effect;
        int channel;
        Mixer::State state;
    };

    Sound_effect_handle(Mixer&, const Sound_effect&, int channel);

    std::shared_ptr<Impl_> ptr_;
};

} // end namespace audio

} // end namespace ge211