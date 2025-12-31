/**
 * Dee Dee Text-to-Speech System
 * Uses original audio soundbites when available, falls back to Web Speech API TTS
 */

const deedeeTTS = {
    synth: null,
    voices: [],
    voiceMap: {}, // Maps voice characteristics to actual voices
    initialized: false,
    enabled: true, // Enabled by default
    audioCache: {}, // Cache for audio elements
    useAudioFiles: true, // Use original soundbites when available
    
    // Base path for audio files (relative to app root)
    audioBasePath: 'assets/audio/soundbites/',
    
    // Map phrases to audio file names (without extension)
    audioFileMap: {
        // Terminator
        "i'll be back": "terminator_ill_be_back.mp3",
        "hasta la vista, baby": "terminator_hasta_la_vista.mp3",
        "i need your clothes, your boots, and your motorcycle": "terminator_clothes.mp3",
        "come with me if you want to live": "terminator_come_with_me.mp3",
        
        // Star Wars
        "may the force be with you": "starwars_may_the_force.mp3",
        "use the force, luke": "starwars_use_the_force.mp3",
        "i am your father": "starwars_i_am_your_father.mp3",
        "these aren't the droids you're looking for": "starwars_droids.mp3",
        "help me, obi-wan kenobi, you're my only hope": "starwars_help_me_obiwan.mp3",
        "i've got a bad feeling about this": "starwars_bad_feeling.mp3",
        "do or do not, there is no try": "starwars_do_or_do_not.mp3",
        "it's a trap": "starwars_its_a_trap.mp3",
        "hello there": "starwars_hello_there.mp3",
        
        // Star Trek
        "live long and prosper": "startrek_live_long.mp3",
        "make it so": "startrek_make_it_so.mp3",
        "engage": "startrek_engage.mp3",
        "beam me up, scotty": "startrek_beam_me_up.mp3",
        "resistance is futile": "startrek_resistance.mp3",
        "set phasers to stun": "startrek_phasers.mp3",
        
        // Mario
        "it's a me, mario": "mario_its_a_me.mp3",
        "here we go": "mario_here_we_go.mp3",
        
        // Groot
        "i am groot": "groot_i_am_groot.mp3",
        
        // The Simpsons
        "d'oh": "simpsons_doh.mp3",
        "don't have a cow, man": "simpsons_dont_have_cow.mp3",
        "eat my shorts": "simpsons_eat_my_shorts.mp3",
        "excellent": "simpsons_excellent.mp3",
        "release the hounds": "simpsons_release_hounds.mp3",
        "ay caramba": "simpsons_ay_caramba.mp3",
        
        // Batman
        "i'm batman": "batman_im_batman.mp3",
        "to the batmobile": "batman_batmobile.mp3",
        "to the batcave": "batman_batcave.mp3",
        "same bat-time, same bat-channel": "batman_same_bat_time.mp3",
        
        // Friends
        "how you doin'": "friends_how_you_doin.mp3",
        "we were on a break": "friends_on_a_break.mp3",
        "pivot": "friends_pivot.mp3",
        "could i be wearing any more clothes": "friends_clothes.mp3",
        "smelly cat, smelly cat": "friends_smelly_cat.mp3",
        
        // Other popular quotes
        "to infinity and beyond": "toystory_infinity.mp3",
        "you've got a friend in me": "toystory_friend.mp3",
        "hakuna matata": "lionking_hakuna.mp3",
        "let it go": "frozen_let_it_go.mp3",
        "as you wish": "princessbride_as_you_wish.mp3",
        "inconceivable": "princessbride_inconceivable.mp3",
        "just keep swimming": "nemo_just_keep_swimming.mp3",
        "fish are friends, not food": "nemo_friends_not_food.mp3",
        "mine! mine! mine": "nemo_mine.mp3",
        "great scott": "backtothefuture_great_scott.mp3",
        "1.21 gigawatts": "backtothefuture_gigawatts.mp3",
        "roads? where we're going, we don't need roads": "backtothefuture_roads.mp3",
    },
    
    // Check if TTS is enabled in settings
    isEnabled() {
        const saved = localStorage.getItem('aide-tts-enabled');
        if (saved !== null) {
            this.enabled = saved === 'true';
        }
        return this.enabled;
    },
    
    
    // Enable/disable TTS
    setEnabled(enabled) {
        this.enabled = enabled;
        localStorage.setItem('aide-tts-enabled', enabled.toString());
    },
    
    // Voice preferences by accent/type
    voicePreferences: {
        robot: ['Alex', 'Samantha', 'Victoria', 'Daniel', 'Google UK English Male', 'Microsoft Zira'],
        australian: ['Google Australian English', 'Karen', 'Microsoft Zira'],
        british: ['Google UK English', 'Daniel', 'Microsoft Hazel', 'Google UK English Male'],
        canadian: ['Google US English', 'Samantha', 'Victoria'],
        western: ['Google US English', 'Alex', 'Daniel'],
        // Character-specific voices
        terminator: ['Google US English', 'Alex', 'Daniel'], // Deep, Austrian accent simulation
        mario: ['Google Italian', 'Samantha', 'Victoria'], // Italian accent
        groot: ['Google US English', 'Alex', 'Daniel'], // Very deep, slow
        simpsons: ['Google US English', 'Samantha', 'Victoria'], // Cartoon-like
        starwars: ['Google UK English', 'Daniel', 'Microsoft Hazel'], // British accents
        startrek: ['Google UK English', 'Daniel', 'Microsoft Hazel'], // British accents
        batman: ['Google US English', 'Alex', 'Daniel'], // Deep, gravelly
        default: ['Samantha', 'Victoria', 'Alex', 'Google US English']
    },
    
    // Character voice settings (pitch, rate, volume adjustments)
    characterSettings: {
        terminator: { pitch: 0.6, rate: 0.75, volume: 1.0 }, // Deep, slow (Arnold)
        mario: { pitch: 1.3, rate: 1.1, volume: 0.9 }, // Higher pitch, faster (Italian)
        groot: { pitch: 0.5, rate: 0.6, volume: 0.9 }, // Very deep, very slow
        simpsons: { pitch: 1.1, rate: 1.0, volume: 0.9 }, // Slightly higher pitch
        starwars: { pitch: 1.0, rate: 0.95, volume: 0.9 }, // British accent
        startrek: { pitch: 1.0, rate: 0.9, volume: 0.9 }, // British accent, slower
        batman: { pitch: 0.7, rate: 0.85, volume: 0.95 }, // Deep, gravelly
        robot: { pitch: 0.8, rate: 0.9, volume: 0.9 },
        western: { pitch: 0.9, rate: 0.85, volume: 0.9 },
        default: { pitch: 1.0, rate: 1.0, volume: 0.8 }
    },
    
    /**
     * Initialize TTS system and load voices
     */
    init() {
        // Load audio file preference
        this.shouldUseAudioFiles();
        
        if (this.initialized) return Promise.resolve();
        
        this.synth = window.speechSynthesis;
        
        // Load voices
        return new Promise((resolve) => {
            const loadVoices = () => {
                this.voices = this.synth.getVoices();
                
                if (this.voices.length === 0) {
                    // Voices might not be loaded yet, try again
                    setTimeout(loadVoices, 100);
                    return;
                }
                
                // Map voices by characteristics
                this.buildVoiceMap();
                this.initialized = true;
                console.log(`✅ TTS initialized with ${this.voices.length} voices`);
                resolve();
            };
            
            // Try loading immediately
            loadVoices();
            
            // Also listen for voiceschanged event
            this.synth.onvoiceschanged = loadVoices;
        });
    },
    
    /**
     * Build a map of voice characteristics to actual voices
     */
    buildVoiceMap() {
        this.voiceMap = {
            robot: this.findVoice(this.voicePreferences.robot),
            australian: this.findVoice(this.voicePreferences.australian),
            british: this.findVoice(this.voicePreferences.british),
            canadian: this.findVoice(this.voicePreferences.canadian),
            western: this.findVoice(this.voicePreferences.western),
            terminator: this.findVoice(this.voicePreferences.terminator),
            mario: this.findVoice(this.voicePreferences.mario),
            groot: this.findVoice(this.voicePreferences.groot),
            simpsons: this.findVoice(this.voicePreferences.simpsons),
            starwars: this.findVoice(this.voicePreferences.starwars),
            startrek: this.findVoice(this.voicePreferences.startrek),
            batman: this.findVoice(this.voicePreferences.batman),
            default: this.findVoice(this.voicePreferences.default)
        };
    },
    
    /**
     * Find the first available voice from a preference list
     */
    findVoice(preferences) {
        for (const pref of preferences) {
            const voice = this.voices.find(v => 
                v.name.includes(pref) || 
                v.lang.includes(pref) ||
                v.voiceURI.includes(pref)
            );
            if (voice) return voice;
        }
        // Fallback to first available voice
        return this.voices[0] || null;
    },
    
    /**
     * Determine voice type from phrase text - matches original character voices
     */
    detectVoiceType(phrase) {
        const lower = phrase.toLowerCase();
        
        // Terminator (Arnold Schwarzenegger) - deep, slow, Austrian accent
        if (lower.includes("i'll be back") || 
            lower.includes("hasta la vista") || 
            lower.includes("i need your clothes") ||
            lower.includes("come with me if you want to live")) {
            return 'terminator';
        }
        
        // Mario - Italian accent, higher pitch
        if (lower.includes("it's a me, mario") || 
            lower.includes("here we go") ||
            lower.includes("mario")) {
            return 'mario';
        }
        
        // Groot - very deep, very slow
        if (lower.includes("i am groot") || 
            lower === "i am groot!") {
            return 'groot';
        }
        
        // The Simpsons - cartoon-like, slightly higher pitch
        if (lower.includes("d'oh") || 
            lower.includes("don't have a cow") ||
            lower.includes("eat my shorts") ||
            lower.includes("excellent") ||
            lower.includes("release the hounds") ||
            lower.includes("ay caramba")) {
            return 'simpsons';
        }
        
        // Star Wars - British accents
        if (lower.includes("may the force be with you") ||
            lower.includes("use the force") ||
            lower.includes("i am your father") ||
            lower.includes("these aren't the droids") ||
            lower.includes("help me, obi-wan") ||
            lower.includes("i've got a bad feeling") ||
            lower.includes("do or do not") ||
            lower.includes("it's a trap") ||
            lower.includes("hello there") ||
            lower.includes("general kenobi")) {
            return 'starwars';
        }
        
        // Star Trek - British accents, slower
        if (lower.includes("live long and prosper") ||
            lower.includes("make it so") ||
            lower.includes("engage") ||
            lower.includes("beam me up") ||
            lower.includes("resistance is futile") ||
            lower.includes("set phasers")) {
            return 'startrek';
        }
        
        // Batman - deep, gravelly
        if (lower.includes("i'm batman") ||
            lower.includes("to the batmobile") ||
            lower.includes("to the batcave") ||
            lower.includes("same bat-time") ||
            lower.includes("holy") && lower.includes("batman")) {
            return 'batman';
        }
        
        // Robot sounds
        if (phrase.includes('*beep*') || phrase.includes('*boop*') || phrase.includes('*whistle*')) {
            return 'robot';
        }
        
        // Australian
        if (lower.includes('mate') || lower.includes("g'day")) {
            return 'australian';
        }
        
        // British
        if (lower.includes('innit') || lower.includes('cheerio')) {
            return 'british';
        }
        
        // Canadian
        if (lower.includes(' eh') || lower.endsWith('eh!') || lower.endsWith('eh?')) {
            return 'canadian';
        }
        
        // Western
        if (lower.includes('partner') || lower.includes('huckleberry') || lower.includes('reckon')) {
            return 'western';
        }
        
        return 'default';
    },
    
    /**
     * Get normalized phrase key for audio file lookup
     */
    normalizePhrase(phrase) {
        return phrase
            .toLowerCase()
            .trim()
            .replace(/[!?.]/g, '')
            .replace(/\s+/g, ' ');
    },
    
    /**
     * Find audio file for a phrase
     */
    findAudioFile(phrase) {
        const normalized = this.normalizePhrase(phrase);
        
        // Try exact match first
        if (this.audioFileMap[normalized]) {
            return this.audioFileMap[normalized];
        }
        
        // Try partial matches (for phrases with variations)
        for (const [key, file] of Object.entries(this.audioFileMap)) {
            if (normalized.includes(key) || key.includes(normalized)) {
                return file;
            }
        }
        
        return null;
    },
    
    /**
     * Play audio file
     */
    async playAudioFile(filename, options = {}) {
        return new Promise((resolve, reject) => {
            // Check cache first
            if (this.audioCache[filename]) {
                const audio = this.audioCache[filename];
                audio.currentTime = 0;
                audio.volume = options.volume !== undefined ? options.volume : 0.8;
                audio.play()
                    .then(() => {
                        audio.onended = () => resolve();
                        audio.onerror = (e) => reject(e);
                    })
                    .catch(reject);
                return;
            }
            
            // Create new audio element
            const audio = new Audio(`${this.audioBasePath}${filename}`);
            audio.volume = options.volume !== undefined ? options.volume : 0.8;
            
            // Cache it
            this.audioCache[filename] = audio;
            
            audio.onloadeddata = () => {
                audio.play()
                    .then(() => {
                        audio.onended = () => resolve();
                        audio.onerror = (e) => reject(e);
                    })
                    .catch(reject);
            };
            
            audio.onerror = (e) => {
                // File doesn't exist, fall back to TTS
                delete this.audioCache[filename];
                reject(new Error('Audio file not found'));
            };
        });
    },
    
    /**
     * Speak a phrase with appropriate voice
     * Tries to use original audio soundbite first, falls back to TTS
     */
    async speak(phrase, options = {}) {
        // Check if TTS is enabled
        if (!this.isEnabled()) {
            console.log('TTS disabled, skipping speech');
            return;
        }
        
        // Clean phrase (remove markdown-style beeps for actual speech)
        const cleanPhrase = phrase
            .replace(/\*beep\*/g, 'beep')
            .replace(/\*boop\*/g, 'boop')
            .replace(/\*whistle\*/g, 'whistle')
            .replace(/\*beep boop\*/g, 'beep boop')
            .replace(/\*beep boop beep\*/g, 'beep boop beep');
        
        // Try to use original audio soundbite first
        if (this.useAudioFiles) {
            const audioFile = this.findAudioFile(cleanPhrase);
            if (audioFile) {
                try {
                    await this.playAudioFile(audioFile, options);
                    return; // Successfully played audio, exit
                } catch (e) {
                    // Audio file not found or failed, fall through to TTS
                    console.log(`Audio file not available for "${cleanPhrase}", using TTS`);
                }
            }
        }
        
        // Fall back to TTS
        if (!this.isSupported()) {
            console.log('TTS not supported');
            return;
        }
        
        if (!this.initialized) {
            await this.init();
        }
        
        // Cancel any ongoing speech
        this.synth.cancel();
        
        // Detect voice type
        const voiceType = options.voiceType || this.detectVoiceType(phrase);
        const voice = this.voiceMap[voiceType] || this.voiceMap.default;
        
        // Create utterance
        const utterance = new SpeechSynthesisUtterance(cleanPhrase);
        
        // Set voice
        if (voice) {
            utterance.voice = voice;
            utterance.lang = voice.lang;
        }
        
        // Get character-specific settings or use defaults
        const charSettings = this.characterSettings[voiceType] || this.characterSettings.default;
        
        // Set options with character-specific adjustments
        utterance.rate = options.rate !== undefined ? options.rate : charSettings.rate;
        utterance.pitch = options.pitch !== undefined ? options.pitch : charSettings.pitch;
        utterance.volume = options.volume !== undefined ? options.volume : charSettings.volume;
        
        // Additional adjustments for specific characters
        // Terminator: extra deep and slow for Arnold's voice
        if (voiceType === 'terminator') {
            utterance.pitch = Math.max(0.5, utterance.pitch - 0.1); // Even deeper
            utterance.rate = Math.max(0.65, utterance.rate - 0.1); // Even slower
        }
        
        // Groot: extremely deep and slow
        if (voiceType === 'groot') {
            utterance.pitch = 0.4; // Very deep
            utterance.rate = 0.5; // Very slow
        }
        
        // Mario: higher pitch, faster, Italian accent simulation
        if (voiceType === 'mario') {
            utterance.pitch = 1.4; // Higher pitch
            utterance.rate = 1.15; // Faster
        }
        
        // Batman: deep and gravelly
        if (voiceType === 'batman') {
            utterance.pitch = 0.65; // Deep
            utterance.rate = 0.8; // Slightly slower
        }
        
        // Star Wars: British accent (slightly slower, proper enunciation)
        if (voiceType === 'starwars') {
            utterance.rate = 0.92;
            utterance.pitch = 1.0;
        }
        
        // Star Trek: British accent, slower, more formal
        if (voiceType === 'startrek') {
            utterance.rate = 0.85;
            utterance.pitch = 1.0;
        }
        
        // Speak
        return new Promise((resolve, reject) => {
            utterance.onend = () => resolve();
            utterance.onerror = (e) => reject(e);
            this.synth.speak(utterance);
        });
    },
    
    /**
     * Speak a phrase from a category with pop culture flair
     */
    async speakPhrase(category, customPhrase = null) {
        if (!window.deedeePersonality) {
            console.warn('Dee Dee personality not loaded');
            return;
        }
        
        const phrase = customPhrase || window.deedeePersonality.getPhrase(category);
        if (!phrase) return;
        
        await this.speak(phrase);
    },
    
    /**
     * Speak with enhancement (adds pop culture phrase to message)
     */
    async speakEnhanced(message, context = 'helping') {
        if (!window.deedeePersonality) {
            await this.speak(message);
            return;
        }
        
        const enhanced = window.deedeePersonality.enhanceMessage(message, context);
        await this.speak(enhanced);
    },
    
    /**
     * Stop current speech and audio playback
     */
    stop() {
        // Stop TTS
        if (this.synth) {
            this.synth.cancel();
        }
        
        // Stop all audio playback
        for (const audio of Object.values(this.audioCache)) {
            if (audio && !audio.paused) {
                audio.pause();
                audio.currentTime = 0;
            }
        }
    },
    
    /**
     * Enable/disable use of audio files (fallback to TTS if disabled)
     */
    setUseAudioFiles(useAudio) {
        this.useAudioFiles = useAudio;
        localStorage.setItem('aide-tts-use-audio-files', useAudio.toString());
    },
    
    /**
     * Check if audio files should be used
     */
    shouldUseAudioFiles() {
        const saved = localStorage.getItem('aide-tts-use-audio-files');
        if (saved !== null) {
            this.useAudioFiles = saved === 'true';
        }
        return this.useAudioFiles;
    },
    
    /**
     * Check if TTS is supported
     */
    isSupported() {
        return 'speechSynthesis' in window;
    },
    
    /**
     * Get available voices (for debugging)
     */
    getAvailableVoices() {
        return this.voices.map(v => ({
            name: v.name,
            lang: v.lang,
            default: v.default,
            localService: v.localService
        }));
    }
};

// Make it available globally
if (typeof window !== 'undefined') {
    window.deedeeTTS = deedeeTTS;
}

// Export for Node.js/Electron
if (typeof module !== 'undefined' && module.exports) {
    module.exports = deedeeTTS;
}
