# gm_8bit
A module for manipulating voice data in Garry's Mod.

# What does it do?
gm_8bit is designed to be a starting point for any kind of voice stream manipulation you might want to do on a Garry's Mod server (or any source engine server, with a bit of adjustment).

gm_8bit can decompress and recompress steam voice packets. It includes an SV_BroadcastVoiceData hook to allow server operators to incercept and manipulate this voice data. It makes several things possible, including:
* Relaying server voice data to external locations
* Performing voice recognition and producing transcripts
* Recording voice data in compressed or uncompressed form
* Applying transformation to user voice streams, for example pitch correction, noise suppression, or gain control.

# Changed:
Added many voice presets, reworked sv_broadcastdata. Fixed many bugs introduced by Garry's Mod updates.


# API
`eightbit.EnableBroadcast(bool)` Sets whether the module should relay voice packets to `localhost:4000`.

`eightbit.SetBroadcastIP(string)` Controls what IP the module should relay voice packets to, if broadcast is enabled.

`eightbit.SetBroadcastPort(number)` Controls what port the module should relay voice packets to, if broadcast is enabled.

`eightbit.EnableEffect(userid, number)` Sets whether to enable audio effect for a given userid. Takes an eightbit.EFF enum.

`eightbit.SetGainFactor(number)` Sets the gain multiplier to apply to affected userids.

`eightbit.SetCrushFactor(number)` Sets the bitcrush factor for the reference bitcrush implementation.

`eightbit.SetDesampleRate(number)` Sets the desample multiplier, used by EFF_DESAMPLE.

`eightbit.EFF_NONE` No audio effect.

`eightbit.EFF_DESAMPLE` Bitcrush, hight pass, low pass filter. Radio effect.

`eightbit.EFF_BITCRUSH` Distortion, high pass low pass filter, voice of the elite squad of cleaners.

`eightbit.EFF_COMB`       Applies a combine filter, ring modulation, and pitch shifting. Produces a metallic, resonant, low-pitched voice with a “subharmonic” character.

`eightbit.EFF_DARTHVADER` Lowers the pitch by averaging adjacent samples and adds a heavy low?frequency rumble. Creates a deep, menacing voice reminiscent of a certain Sith Lord.

`eightbit.EFF_RADIO`      Adds a short slapback delay (echo) followed by heavy quantization. Emulates the sound of a crackling, low?fidelity walkie?talkie.

`eightbit.EFF_ROBOT`      Lowers pitch significantly and layers a doubled, slightly distorted signal. Produces a classic monotone robotic voice.

`eightbit.EFF_ALIEN`      Raises pitch with a fast vibrato, adds a faint doubling effect, and cuts low frequencies. Results in a thin, warbling, extraterrestrial?like voice.

`eightbit.EFF_OVERDRIVE`  Applies aggressive soft?clipping (tanh) with high pre?gain and a short, dirty delay. Gives the voice a thick, saturated, tube?amp overdrive character.

`eightbit.EFF_DISTORTION` Heavy, hard?clipping distortion with high gain. Creates a raw, “fuzzy” sound similar to a fuzz pedal.

`eightbit.EFF_TELEPHONE`  Band?pass filters (500?Hz – 3.2?kHz), moderate bit?crushing, and occasional signal glitches. Mimics the narrow bandwidth and artifacts of a traditional phone line.

`eightbit.EFF_MEGAPHONE`  Aggressive band?pass filtering, a short metallic slapback, and hard limiting. Produces the strained, hollow sound of a public address megaphone.

`eightbit.EFF_CHIPMUNK`   Speeds up the audio by skipping every other sample, raising pitch roughly one octave. Creates the familiar “chipmunk” high?pitched voice.

`eightbit.EFF_SLOWMOTION` Slows down the audio by repeating each sample, lowering pitch by about one octave. Gives a deep, stretched, “slow?motion” effect.

`Eightbit.EFF_COMBO` is a hybrid of ring modulation, soft limiting, comb filtering, and bandpass filtering. Creates a "Metrocop" effect.

# Edited by RG Studio and Quantum Ocean projects ?

# Original module - https://github.com/Meachamp/gm_8bit
# The module taken as a base - https://github.com/Devinsideer/gm_8bit2
