//MIDI======================================================================
    void handleNoteOn(byte inChannel, byte inNote, byte inVelocity);

    void handleNoteOff(byte inChannel, byte inNote, byte inVelocity);

    void handleClock();

    void handleStart();

    void handleContinue();

    void handleStop();

    void killNotes();