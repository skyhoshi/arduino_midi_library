#include "unit-tests.h"
#include <src/MIDI.h>
#include <test/mocks/test-mocks_SerialMock.h>

BEGIN_MIDI_NAMESPACE

END_MIDI_NAMESPACE

// -----------------------------------------------------------------------------

BEGIN_UNNAMED_NAMESPACE

template<bool RunningStatus, bool OneByteParsing>
struct VariableSettings : public midi::DefaultSettings
{
    static const bool UseRunningStatus = RunningStatus;
    static const bool Use1ByteParsing  = OneByteParsing;
};

template<bool A, bool B>
const bool VariableSettings<A, B>::UseRunningStatus;
template<bool A, bool B>
const bool VariableSettings<A, B>::Use1ByteParsing;

using namespace testing;
typedef test_mocks::SerialMock<32> SerialMock;
typedef midi::MidiInterface<SerialMock> MidiInterface;

// --

TEST(MidiOutput, sendGenericSingle)
{
    SerialMock serial;
    MidiInterface midi(serial);
    test_mocks::uint8 buffer[3] = { 0 };

    midi.begin();
    midi.send(midi::NoteOn, 47, 42, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 3);
    serial.mTxBuffer.read(buffer, 3);
    EXPECT_THAT(buffer, ElementsAre(0x9b, 47, 42));
}

TEST(MidiOutput, sendGenericWithRunningStatus)
{
    SerialMock serial;
    MidiInterface midi(serial);
    test_mocks::uint8 buffer[5] = { 0 };

    midi.begin();
    EXPECT_EQ(MidiInterface::Settings::UseRunningStatus, true);
    EXPECT_EQ(serial.mTxBuffer.isEmpty(), true);
    midi.send(midi::NoteOn, 47, 42, 12);
    midi.send(midi::NoteOn, 42, 47, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 5);
    serial.mTxBuffer.read(buffer, 5);
    EXPECT_THAT(buffer, ElementsAre(0x9b, 47, 42, 42, 47));
}

TEST(MidiOutput, sendGenericWithoutRunningStatus)
{
    typedef VariableSettings<false, true> Settings; // No running status
    typedef midi::MidiInterface<SerialMock, Settings> MidiInterface;

    SerialMock serial;
    MidiInterface midi(serial);
    test_mocks::uint8 buffer[6] = { 0 };

    // Same status byte
    midi.begin();
    EXPECT_EQ(MidiInterface::Settings::UseRunningStatus, false);
    EXPECT_EQ(serial.mTxBuffer.isEmpty(), true);
    midi.send(midi::NoteOn, 47, 42, 12);
    midi.send(midi::NoteOn, 42, 47, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 6);
    serial.mTxBuffer.read(buffer, 6);
    EXPECT_THAT(buffer, ElementsAre(0x9b, 47, 42, 0x9b, 42, 47));

    // Different status byte
    midi.begin();
    midi.send(midi::NoteOn,  47, 42, 12);
    midi.send(midi::NoteOff, 47, 42, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 6);
    serial.mTxBuffer.read(buffer, 6);
    EXPECT_THAT(buffer, ElementsAre(0x9b, 47, 42, 0x8b, 47, 42));
}

TEST(MidiOutput, sendGenericBreakingRunningStatus)
{
    SerialMock serial;
    MidiInterface midi(serial);
    test_mocks::uint8 buffer[6] = { 0 };

    midi.begin();
    midi.send(midi::NoteOn,  47, 42, 12);
    midi.send(midi::NoteOff, 47, 42, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 6);
    serial.mTxBuffer.read(buffer, 6);
    EXPECT_THAT(buffer, ElementsAre(0x9b, 47, 42, 0x8b, 47, 42));
}

// --

TEST(MidiOutput, sendNoteOn)
{
    SerialMock serial;
    MidiInterface midi(serial);
    test_mocks::uint8 buffer[5] = { 0 };

    midi.begin();
    midi.sendNoteOn(10, 11, 12);
    midi.sendNoteOn(12, 13, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 5);
    serial.mTxBuffer.read(buffer, 5);
    EXPECT_THAT(buffer, ElementsAre(0x9b, 10, 11, 12, 13));
}

TEST(MidiOutput, sendNoteOff)
{
    SerialMock serial;
    MidiInterface midi(serial);
    test_mocks::uint8 buffer[5] = { 0 };

    midi.begin();
    midi.sendNoteOff(10, 11, 12);
    midi.sendNoteOff(12, 13, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 5);
    serial.mTxBuffer.read(buffer, 5);
    EXPECT_THAT(buffer, ElementsAre(0x8b, 10, 11, 12, 13));
}

TEST(MidiOutput, sendProgramChange)
{
    SerialMock serial;
    MidiInterface midi(serial);
    test_mocks::uint8 buffer[3] = { 0 };

    midi.begin();
    midi.sendProgramChange(42, 12);
    midi.sendProgramChange(47, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 3);
    serial.mTxBuffer.read(buffer, 3);
    EXPECT_THAT(buffer, ElementsAre(0xcb, 42, 47));
}

TEST(MidiOutput, sendControlChange)
{
    SerialMock serial;
    MidiInterface midi(serial);
    test_mocks::uint8 buffer[5] = { 0 };

    midi.begin();
    midi.sendControlChange(42, 12, 12);
    midi.sendControlChange(47, 12, 12);
    EXPECT_EQ(serial.mTxBuffer.getLength(), 5);
    serial.mTxBuffer.read(buffer, 5);
    EXPECT_THAT(buffer, ElementsAre(0xbb, 42, 12, 47, 12));
}

TEST(MidiOutput, sendPitchBend)
{
    SerialMock serial;
    MidiInterface midi(serial);
    test_mocks::uint8 buffer[7] = { 0 };

    // Int signature - arbitrary values
    {
        midi.begin();
        midi.sendPitchBend(0, 12);
        midi.sendPitchBend(100, 12);
        midi.sendPitchBend(-100, 12);
        EXPECT_EQ(serial.mTxBuffer.getLength(), 7);
        serial.mTxBuffer.read(buffer, 7);
        EXPECT_THAT(buffer, ElementsAre(0xeb,
                                        0x00, 0x40,
                                        0x64, 0x40,
                                        0x1c, 0x3f));
    }
    // Int signature - min/max
    {
        midi.begin();
        midi.sendPitchBend(0, 12);
        midi.sendPitchBend(MIDI_PITCHBEND_MAX, 12);
        midi.sendPitchBend(MIDI_PITCHBEND_MIN, 12);
        EXPECT_EQ(serial.mTxBuffer.getLength(), 7);
        serial.mTxBuffer.read(buffer, 7);
        EXPECT_THAT(buffer, ElementsAre(0xeb,
                                        0x00, 0x40,
                                        0x7f, 0x7f,
                                        0x00, 0x00));
    }
    // Float signature
    {
        midi.begin();
        midi.sendPitchBend(0.0,  12);
        midi.sendPitchBend(1.0,  12);
        midi.sendPitchBend(-1.0, 12);
        EXPECT_EQ(serial.mTxBuffer.getLength(), 7);
        serial.mTxBuffer.read(buffer, 7);
        EXPECT_THAT(buffer, ElementsAre(0xeb,
                                        0x00, 0x40,
                                        0x7f, 0x7f,
                                        0x00, 0x00));
    }
}

TEST(MidiOutput, sendPolyPressure)
{

}

TEST(MidiOutput, sendAfterTouch)
{

}

TEST(MidiOutput, sendSysEx)
{

}

TEST(MidiOutput, sendTimeCodeQuarterFrame)
{

}

TEST(MidiOutput, sendSongPosition)
{

}

TEST(MidiOutput, sendSongSelect)
{

}

TEST(MidiOutput, sendTuneRequest)
{

}

TEST(MidiOutput, sendRealTime)
{

}

END_UNNAMED_NAMESPACE