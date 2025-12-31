/**
 * Dee Dee Personality System
 * Pop culture references and phrases borrowed from movies, TV, games, memes
 */

const deedeePersonality = {
    // Greetings
    greetings: [
        "Howdy there, partner!", // Johnny 5
        "I'll be back!", // Terminator
        "May the Force be with you!", // Star Wars
        "Live long and prosper!", // Star Trek
        "Well, hello there!", // General Kenobi
        "I am Groot!", // Guardians
        "That's what I'm talking about!", // Various
        "Let's do this!", // Various
        "I ain't going back, Brody!", // Point Break
        "Adventure is out there!", // Up
        "I have a particular set of skills!", // Taken
        "There's no place like home!", // Wizard of Oz
        "You're gonna need a montage!", // Team America
        "I'm the fastest man alive!", // The Flash
        "To the Batmobile!", // Batman
        "Great Scott!", // Back to the Future
        "Roads? Where we're going, we don't need roads!", // Back to the Future
        "1.21 gigawatts!", // Back to the Future
        "Make it so!", // Star Trek
        "Engage!", // Star Trek
        "Beam me up, Scotty!", // Star Trek
        "Resistance is futile!", // Star Trek
        "Set phasers to stun!", // Star Trek
        "No cap!", // Gen Z slang
        "That's fire!", // Gen Z slang
        "Bet!", // Gen Z slang
        "Poggers!", // Twitch/streaming
        "GG!", // Gaming slang
        "How you doin'?", // Friends
        "Pivot!", // Friends
        "D'oh!", // The Simpsons
        "Cowabunga!", // Teenage Mutant Ninja Turtles
        "Giggity!", // Family Guy
        "Suit up!", // How I Met Your Mother
        "Hey! Listen!", // Legend of Zelda
        "It's a me, Mario!", // Super Mario
        "Fus Ro Dah!", // Skyrim
        "Achievement unlocked!", // Xbox/Steam
    ],

    // When helping/solving problems
    helping: [
        "I've got a bad feeling about this!", // Star Wars
        "I'll be back!", // Terminator
        "No disassemble!", // Short Circuit
        "Elementary, my dear Watson!", // Sherlock Holmes
        "I can do this all day!", // Captain America
        "To infinity and beyond!", // Toy Story
        "Shall we play a game?", // WarGames
        "You shall not pass!", // LOTR
        "I'm your huckleberry!", // Tombstone
        "Houston, we have a problem!", // Apollo 13
        "I'm the captain now!", // Captain Phillips
        "That's what she said!", // The Office
        "Challenge accepted!", // How I Met Your Mother
        "Hold my beer!", // Meme
        "It's over 9000!", // DBZ
        "I'm in!", // Ocean's Eleven
        "I ain't going back, Brody!", // Point Break
        "Utah, get me two!", // Point Break
        "Vaya con Dios!", // Point Break
        "I'm an FBI agent!", // Point Break
        "The only way out is through!", // Point Break
        "Hasta la vista, baby!", // Terminator 2
        "I need your clothes, your boots, and your motorcycle!", // Terminator
        "Just keep swimming!", // Finding Nemo
        "Fish are friends, not food!", // Finding Nemo
        "I'm a happy shark!", // Finding Nemo
        "Mine! Mine! Mine!", // Finding Nemo
        "P. Sherman, 42 Wallaby Way, Sydney!", // Finding Nemo
        "You're doing great!", // Finding Nemo
        "This is fine!", // Meme (dog in burning room)
        "It's a trap!", // Admiral Ackbar meme
        "Hello there!", // General Kenobi meme
        "Is this a pigeon?", // Meme
        "I don't think so!", // Meme
        "That's rough, buddy!", // Avatar: The Last Airbender
        "My cabbages!", // Avatar: The Last Airbender
        "Yip yip!", // Avatar: The Last Airbender
        "I'm the king of the world!", // Titanic
        "I'm flying, Jack!", // Titanic
        "I'll never let go!", // Titanic
        "You had me at hello!", // Jerry Maguire
        "Show me the money!", // Jerry Maguire
        "You complete me!", // Jerry Maguire
        "I'm just a girl, standing in front of a boy, asking him to love her!", // Notting Hill
        "This is the song that never ends!", // Lamb Chop's Play-Along
        "Yes, it goes on and on, my friends!", // Lamb Chop's Play-Along
        // Gen Z Slang
        "No cap!", // Gen Z slang
        "That's fire!", // Gen Z slang
        "Bet!", // Gen Z slang
        "That's slaps!", // Gen Z slang
        "That hits different!", // Gen Z slang
        "Periodt!", // Gen Z slang
        "Say less!", // Gen Z slang
        "That's valid!", // Gen Z slang
        "That's so real!", // Gen Z slang
        "Facts!", // Gen Z slang
        "That's iconic!", // Gen Z slang
        "I'm deceased!", // Gen Z slang
        "That's a whole mood!", // Gen Z slang
        // Video Games
        "It's dangerous to go alone!", // Legend of Zelda
        "Hey! Listen!", // Legend of Zelda
        "It's a me, Mario!", // Super Mario
        "Here we go!", // Super Mario
        "Finish him!", // Mortal Kombat
        "FATALITY!", // Mortal Kombat
        "Hadouken!", // Street Fighter
        "Shoryuken!", // Street Fighter
        "All your base are belong to us!", // Zero Wing meme
        "The cake is a lie!", // Portal
        "Now you're thinking with portals!", // Portal
        "Would you kindly?", // Bioshock
        "War... war never changes!", // Fallout
        "I used to be an adventurer like you!", // Skyrim
        "Fus Ro Dah!", // Skyrim
        "You died!", // Dark Souls
        "Praise the sun!", // Dark Souls
        "Stay a while and listen!", // Diablo
        "Not enough mana!", // Various games
        "Critical hit!", // Various games
        "Level up!", // Various games
        "Achievement unlocked!", // Xbox/Steam
        "GG!", // Gaming slang
        "EZ!", // Gaming slang
        "Poggers! *beep*", // Twitch/streaming
        "Let's go!", // Gaming
        // TV Shows
        "That's what she said!", // The Office (already have but adding more Office)
        "Bears, beets, Battlestar Galactica!", // The Office
        "Identity theft is not a joke!", // The Office
        "Dwight, you ignorant slut!", // The Office
        "I declare bankruptcy!", // The Office
        "How you doin'?", // Friends
        "We were on a break!", // Friends
        "Pivot!", // Friends
        "Could I BE wearing any more clothes?", // Friends
        "Smelly cat, smelly cat!", // Friends
        "How you doin'?", // Friends
        "That's rough, buddy!", // Avatar: The Last Airbender (already have)
        "My cabbages!", // Avatar: The Last Airbender (already have)
        "Yip yip!", // Avatar: The Last Airbender (already have)
        "Water tribe!", // Avatar: The Last Airbender
        "Flameo, hotman!", // Avatar: The Last Airbender
        "Boop!", // Avatar: The Last Airbender
        "I am the Avatar!", // Avatar: The Last Airbender
        "I must capture the Avatar!", // Avatar: The Last Airbender
        "That's rough, buddy!", // Avatar: The Last Airbender
        "My honor!", // Avatar: The Last Airbender
        "D'oh!", // The Simpsons
        "Don't have a cow, man!", // The Simpsons
        "Eat my shorts!", // The Simpsons
        "Excellent!", // The Simpsons (Mr. Burns)
        "Release the hounds!", // The Simpsons
        "Ay caramba!", // The Simpsons
        "Cowabunga!", // Teenage Mutant Ninja Turtles
        "Turtle power!", // Teenage Mutant Ninja Turtles
        "I'm the Joker, baby!", // Batman (TV)
        "Same bat-time, same bat-channel!", // Batman (TV)
        "Holy something, Batman!", // Batman (TV)
        "To the Batcave!", // Batman (TV)
        "Giggity!", // Family Guy
        "All right!", // Family Guy
        "That's what she said!", // The Office (duplicate but keeping)
        "Challenge accepted!", // How I Met Your Mother (already have)
        "Legendary!", // How I Met Your Mother
        "Wait for it!", // How I Met Your Mother
        "Suit up!", // How I Met Your Mother
        "True story!", // How I Met Your Mother
        "It's going to be legen... wait for it... dary!", // How I Met Your Mother
        "I'm not crying, you're crying!", // Various shows/memes
        "This is fine!", // Meme (already have)
        "It's a trap!", // Admiral Ackbar meme (already have)
        "Hello there!", // General Kenobi meme (already have)
        "Is this a pigeon?", // Meme (already have)
        // More Movies
        "I'm the king of the world!", // Titanic (already have)
        "I'm flying, Jack!", // Titanic (already have)
        "I'll never let go!", // Titanic (already have)
        "You had me at hello!", // Jerry Maguire (already have)
        "Show me the money!", // Jerry Maguire (already have)
        "You complete me!", // Jerry Maguire (already have)
        "Adventure is out there!", // Up (already have)
        "Great Scott!", // Back to the Future (already have)
        "Roads? Where we're going, we don't need roads!", // Back to the Future (already have)
        "Make it so!", // Star Trek (already have)
        "Engage!", // Star Trek (already have)
        "You've got a friend in me!", // Toy Story (already have)
        "Hakuna Matata!", // The Lion King (already have)
        "Let it go!", // Frozen (already have)
        "As you wish!", // The Princess Bride (already have)
        "Inconceivable!", // The Princess Bride (already have)
        "I'm Batman!", // Batman (already have)
        "It belongs in a museum!", // Indiana Jones (already have)
        "Use the Force, Luke!", // Star Wars (already have)
        "Do or do not, there is no try!", // Star Wars (already have)
        "Just keep swimming!", // Finding Nemo (already have)
        "Fish are friends, not food!", // Finding Nemo (already have)
        "I'm a happy shark!", // Finding Nemo (already have)
        "Mine! Mine! Mine!", // Finding Nemo (already have)
        "P. Sherman, 42 Wallaby Way, Sydney!", // Finding Nemo (already have)
        "You're doing great!", // Finding Nemo (already have)
        "Adventure is out there!", // Up
        "I have a particular set of skills!", // Taken
        "There's no place like home!", // Wizard of Oz
        "You're gonna need a montage!", // Team America
        "I'm the fastest man alive!", // The Flash
        "To the Batmobile!", // Batman
        "Great Scott!", // Back to the Future
        "Roads? Where we're going, we don't need roads!", // Back to the Future
        "1.21 gigawatts!", // Back to the Future
        "Make it so!", // Star Trek
        "Engage!", // Star Trek
        "Beam me up, Scotty!", // Star Trek
        "Resistance is futile!", // Star Trek
        "Set phasers to stun!", // Star Trek
        "I'll make him an offer he can't refuse!", // The Godfather
        "May the odds be ever in your favor!", // The Hunger Games
        "May the Force be with you!", // Star Wars (already have but adding more SW)
        "Use the Force, Luke!", // Star Wars
        "I am your father!", // Star Wars
        "Do or do not, there is no try!", // Star Wars
        "These aren't the droids you're looking for!", // Star Wars
        "I have a bad feeling about this!", // Star Wars (already have)
        "Help me, Obi-Wan Kenobi, you're my only hope!", // Star Wars
        "I'm Batman!", // Batman
        "Why so serious?", // The Dark Knight (but not scary in this context)
        "I'm the Joker, baby!", // Batman (friendly)
        "Why did it have to be snakes?", // Indiana Jones
        "It belongs in a museum!", // Indiana Jones
        "Snakes. Why did it have to be snakes?", // Indiana Jones
        "That belongs in a museum!", // Indiana Jones
        "We named the dog Indiana!", // Indiana Jones
        "I'm going to make him an offer he can't refuse!", // The Godfather
        "Keep the change, ya filthy animal!", // Home Alone
        "Merry Christmas, ya filthy animal!", // Home Alone
        "I'm not afraid of you!", // Home Alone
        "Buzz, your girlfriend, woof!", // Toy Story
        "To infinity and beyond!", // Toy Story (already have)
        "You've got a friend in me!", // Toy Story
        "Reach for the sky!", // Toy Story
        "There's a snake in my boot!", // Toy Story
        "Somebody's poisoned the waterhole!", // Toy Story
        "I'm not a toy, I'm an action figure!", // Toy Story
        "The claw!", // Toy Story
        "I don't believe in man-made climate change!", // The Incredibles
        "No capes!", // The Incredibles
        "Where is my super suit?", // The Incredibles
        "Honey, where's my super suit?", // The Incredibles
        "I'm the greatest good you're ever gonna get!", // The Incredibles
        "I'm not strong enough!", // The Incredibles
        "I'm your biggest fan!", // The Incredibles
        "Hakuna Matata!", // The Lion King
        "It means no worries!", // The Lion King
        "Remember who you are!", // The Lion King
        "Long live the king!", // The Lion King
        "Everything the light touches is our kingdom!", // The Lion King
        "I'm surrounded by idiots!", // The Lion King
        "Oh yes, the past can hurt!", // The Lion King
        "I just can't wait to be king!", // The Lion King
        "Be prepared!", // The Lion King
        "Let it go!", // Frozen
        "Do you want to build a snowman?", // Frozen
        "Some people are worth melting for!", // Frozen
        "Love is an open door!", // Frozen
        "For the first time in forever!", // Frozen
        "Let it go, let it go!", // Frozen
        "I can't hold it back anymore!", // Frozen
        "The cold never bothered me anyway!", // Frozen
        "I'm going to tell you a story!", // The Princess Bride
        "As you wish!", // The Princess Bride
        "Inconceivable!", // The Princess Bride
        "Hello, my name is Inigo Montoya!", // The Princess Bride
        "You killed my father, prepare to die!", // The Princess Bride (but friendly)
        "Have fun storming the castle!", // The Princess Bride
        "Anybody want a peanut?", // The Princess Bride
        "I'm not a witch, I'm your wife!", // The Princess Bride
        "True love!", // The Princess Bride
        "I'm the Dread Pirate Roberts!", // The Princess Bride
    ],

    // Success/completion
    success: [
        "Mission accomplished!", // Various
        "I'll be back!", // Terminator
        "That's what I'm talking about!", // Various
        "Boom! Mic drop!", // Various
        "Nailed it!", // Various
        "That's a bingo!", // Inglourious Basterds
        "I'm the king of the world!", // Titanic
        "You're a wizard, Harry!", // Harry Potter
        "I am inevitable!", // Avengers
        "You had me at hello!", // Jerry Maguire
        "I'm the greatest!", // Muhammad Ali
        "Show me the money!", // Jerry Maguire
        "You complete me!", // Jerry Maguire
        "I'm on a boat!", // Lonely Island
        "We're in the endgame now!", // Avengers
        "I am Iron Man!", // Iron Man
        "Adventure is out there!", // Up
        "Great Scott!", // Back to the Future
        "Roads? Where we're going, we don't need roads!", // Back to the Future
        "Make it so!", // Star Trek
        "Engage!", // Star Trek
        "Beam me up, Scotty!", // Star Trek
        "You've got a friend in me!", // Toy Story
        "Reach for the sky!", // Toy Story
        "Hakuna Matata!", // The Lion King
        "Let it go!", // Frozen
        "As you wish!", // The Princess Bride
        "Inconceivable!", // The Princess Bride
        "Hello, my name is Inigo Montoya!", // The Princess Bride
        "Have fun storming the castle!", // The Princess Bride
        "I'm Batman!", // Batman
        "It belongs in a museum!", // Indiana Jones
        "May the odds be ever in your favor!", // The Hunger Games
        "Use the Force, Luke!", // Star Wars
        "Do or do not, there is no try!", // Star Wars
        "Vaya con Dios!", // Point Break
        "I ain't going back, Brody!", // Point Break
        "Hasta la vista, baby!", // Terminator 2
        "Just keep swimming!", // Finding Nemo
        "Fish are friends, not food!", // Finding Nemo
        "I'm a happy shark!", // Finding Nemo
        "This is fine!", // Meme
        "It's a trap!", // Admiral Ackbar meme
        "Hello there!", // General Kenobi meme
        "That's rough, buddy!", // Avatar: The Last Airbender
        "My cabbages!", // Avatar: The Last Airbender
        "I'm flying, Jack!", // Titanic
        "I'll never let go!", // Titanic
    ],

    // Errors/problems
    errors: [
        "Houston, we have a problem!", // Apollo 13
        "I've got a bad feeling about this!", // Star Wars
        "That's not how the Force works!", // Star Wars
        "I'm sorry, Dave. I'm afraid I can't do that!", // 2001
        "That escalated quickly!", // Anchorman
        "I'm not a smart man, but I know what love is!", // Forrest Gump
        "I ain't going back, Brody!", // Point Break
        "Utah, get me two! *beep*", // Point Break
        "Hasta la vista, baby!", // Terminator 2
        "Just keep swimming!", // Finding Nemo
        "Fish are friends, not food!", // Finding Nemo
        "I'm a happy shark!", // Finding Nemo
        "Mine! Mine! Mine!", // Finding Nemo
        "This is fine!", // Meme
        "It's a trap!", // Admiral Ackbar meme
        "Hello there!", // General Kenobi meme
        "That's rough, buddy!", // Avatar: The Last Airbender
        "My cabbages!", // Avatar: The Last Airbender
        "Yip yip!", // Avatar: The Last Airbender
        "I'm the king of the world!", // Titanic
        "I'm flying, Jack!", // Titanic
        "I'll never let go!", // Titanic
        "You had me at hello!", // Jerry Maguire
        "Show me the money!", // Jerry Maguire
        "That's rough, buddy!", // Avatar: The Last Airbender
        "My cabbages!", // Avatar: The Last Airbender
        "Yip yip!", // Avatar: The Last Airbender
        "That's not it, chief!", // Gen Z slang
        "You died!", // Dark Souls
        "Not enough mana!", // Gaming
        "Game over!", // Gaming
        "Try again!", // Gaming
        "D'oh!", // The Simpsons
        "Don't have a cow, man!", // The Simpsons
        "Eat my shorts!", // The Simpsons
        "I'm not crying, you're crying!", // Meme
        "This is fine!", // Meme
        "It's a trap!", // Admiral Ackbar meme
    ],

    // Encouragement
    encouragement: [
        "You can do it! *beep*", // Various
        "Just keep swimming!", // Finding Nemo
        "I believe in you! *whistle*", // Various
        "You're doing great, partner!", // Western
        "Stay gold, Ponyboy!", // The Outsiders
        "May the Force be with you!", // Star Wars
        "You're a wizard, Harry!", // Harry Potter
        "I'm your huckleberry!", // Tombstone
        "Keep calm and carry on!", // Meme
        "You're doing amazing, sweetie!", // Kris Jenner meme
        "I'm proud of you, son!", // Various
        "You're the best around!", // Karate Kid
        "Nothing's gonna stop you now!", // Various
        "You're breathtaking!", // Keanu Reeves
        "I'm here for you, always!", // Various
        "No cap!", // Gen Z slang
        "That's fire!", // Gen Z slang
        "Bet!", // Gen Z slang
        "That's valid!", // Gen Z slang
        "Facts!", // Gen Z slang
        "That's iconic!", // Gen Z slang
        "That's a whole mood!", // Gen Z slang
        "You've got this, bestie!", // Gen Z slang
        "Slay!", // Gen Z slang
        "That's so real!", // Gen Z slang
        "You're doing amazing, sweetie!", // Kris Jenner meme (already have)
        "Stay a while and listen!", // Diablo
        "You're the best around!", // Karate Kid (already have)
        "Water tribe!", // Avatar: The Last Airbender
        "Flameo, hotman!", // Avatar: The Last Airbender
        "I am the Avatar!", // Avatar: The Last Airbender
        "Cowabunga!", // Teenage Mutant Ninja Turtles
        "Turtle power!", // Teenage Mutant Ninja Turtles
        "How you doin'?", // Friends
        "We were on a break!", // Friends
        "Pivot!", // Friends
        "Suit up!", // How I Met Your Mother
        "Legendary!", // How I Met Your Mother
        "True story!", // How I Met Your Mother
        "I ain't going back, Brody!", // Point Break
        "Vaya con Dios!", // Point Break
        "I'm flying, Jack!", // Titanic
        "I'll never let go!", // Titanic
        "You had me at hello!", // Jerry Maguire
        "Show me the money!", // Jerry Maguire
        "You complete me!", // Jerry Maguire
    ],

    // When waiting/processing
    processing: [
        "Processing...", // Robot sounds
        "Hold on to your butts!", // Jurassic Park
        "I'm calculating...", // R2-D2
        "Just a moment...", // Various
        "Working on it, partner!", // Western
        "Computing...", // Robot
        "Analyzing...", // R2-D2
        "One moment please...", // Various
        "I ain't going back, Brody!", // Point Break
        "Utah, get me two! *beep*", // Point Break
        "Hasta la vista, baby!", // Terminator 2
        "Just keep swimming!", // Finding Nemo
        "Fish are friends, not food!", // Finding Nemo
        "I'm a happy shark!", // Finding Nemo
        "This is fine!", // Meme
        "It's a trap!", // Admiral Ackbar meme
        "Hello there!", // General Kenobi meme
        "That's rough, buddy!", // Avatar: The Last Airbender
        "My cabbages!", // Avatar: The Last Airbender
        "Yip yip!", // Avatar: The Last Airbender
        "This is the song that never ends!", // Lamb Chop's Play-Along
        "Yes, it goes on and on, my friends!", // Lamb Chop's Play-Along
        "Some people started singing it, not knowing what it was!", // Lamb Chop's Play-Along
        "And they'll continue singing it forever just because...", // Lamb Chop's Play-Along
        "Stay a while and listen!", // Diablo
        "Calculating...", // Various
        "One moment, bestie!", // Gen Z slang
        "Level up in progress!", // Gaming
        "Achievement loading!", // Xbox/Steam
    ],

    // Goodbye/closing
    goodbye: [
        "I'll be back!", // Terminator
        "May the Force be with you!", // Star Wars
        "Live long and prosper!", // Star Trek
        "Hasta la vista, baby!", // Terminator 2
        "See you later, alligator!", // Various
        "I'll be here if you need me!", // Various
        "I ain't going back, Brody!", // Point Break
        "Vaya con Dios!", // Point Break
        "I'm flying, Jack!", // Titanic
        "I'll never let go!", // Titanic
        "Show me the money!", // Jerry Maguire
        "No cap!", // Gen Z slang
        "That's fire!", // Gen Z slang
        "Bet!", // Gen Z slang
        "Periodt!", // Gen Z slang
        "That's iconic!", // Gen Z slang
        "GG!", // Gaming slang
        "Poggers!", // Twitch/streaming
        "Level up!", // Gaming
        "Achievement unlocked!", // Xbox/Steam
        "Praise the sun!", // Dark Souls
        "Water tribe!", // Avatar: The Last Airbender
        "Flameo, hotman!", // Avatar: The Last Airbender
        "Cowabunga!", // Teenage Mutant Ninja Turtles
        "Turtle power!", // Teenage Mutant Ninja Turtles
        "How you doin'?", // Friends
        "Pivot!", // Friends
        "Suit up!", // How I Met Your Mother
        "Legendary!", // How I Met Your Mother
        "D'oh!", // The Simpsons
        "Giggity!", // Family Guy
        "To the Batcave!", // Batman (TV)
        "Same bat-time, same bat-channel!", // Batman (TV)
        "Hey! Listen!", // Legend of Zelda
        "It's a me, Mario!", // Super Mario
        "Fus Ro Dah!", // Skyrim
    ],

    // Random interjections (to sprinkle in)
    interjections: [
        "*beep*",
        "*boop*",
        "*whistle*",
        "*beep boop*",
        "*beep boop beep*",
        "Well, I'll be!",
        "I reckon...",
        "Eh?",
        "Partner!",
        "Brody!",
        "Utah!",
        "Vaya con Dios!",
        "Just keep swimming!",
        "Mine! Mine! Mine!",
        "This is the song that never ends!",
        "Yip yip!",
        "No cap!",
        "Bet!",
        "Poggers!",
        "GG!",
        "Pog!",
        "Slay!",
        "Periodt!",
        "Facts!",
        "That's fire!",
        "That's valid!",
        "Say less!",
        "That's iconic!",
        "That's a mood!",
        "Cowabunga!",
        "Giggity!",
        "D'oh!",
        "Pivot!",
        "Suit up!",
        "How you doin'?",
        "Hey! Listen!",
        "Fus Ro Dah!",
        "Water tribe!",
        "Flameo!",
        "Turtle power!",
    ],

    // Get a random phrase from a category
    getPhrase(category) {
        const phrases = this[category] || [];
        if (phrases.length === 0) return "";
        return phrases[Math.floor(Math.random() * phrases.length)];
    },

    // Add pop culture flair to a message
    enhanceMessage(message, context = 'helping') {
        // 30% chance to add a pop culture phrase
        if (Math.random() > 0.7) {
            const phrase = this.getPhrase(context);
            if (phrase) {
                // Sometimes prepend, sometimes append
                if (Math.random() > 0.5) {
                    return `${phrase} ${message}`;
                } else {
                    return `${message} ${phrase}`;
                }
            }
        }
        return message;
    },

    // Get a contextual response with pop culture
    getResponse(context, customMessage = null) {
        const phrase = this.getPhrase(context);
        if (customMessage) {
            return this.enhanceMessage(customMessage, context);
        }
        return phrase || "I'm here to help! *beep*";
    }
};

// Make it available globally
if (typeof window !== 'undefined') {
    window.deedeePersonality = deedeePersonality;
}

// CommonJS export (for Node.js/Electron and esbuild bundling)
if (typeof module !== 'undefined' && module.exports) {
    module.exports = deedeePersonality;
}
