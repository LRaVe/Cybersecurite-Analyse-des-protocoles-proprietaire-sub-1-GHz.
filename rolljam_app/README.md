# RollJam Attack Tool

## ⚠️ FOR SECURITY RESEARCH AND EDUCATIONAL PURPOSES ONLY ⚠️

This Flipper Zero application demonstrates techniques to **bypass rolling code security** used in car key fobs, garage door openers, and other wireless access systems.

## The RollJam Attack Explained

Rolling codes were designed to prevent simple replay attacks. However, the **RollJam attack** (discovered by Samy Kamkar) can bypass this protection.

### How Rolling Codes Work
```
Normal Operation:
┌─────────────────┐         ┌─────────────────┐
│   Key Fob       │  Code1  │   Car/Garage    │
│   Counter: 5    │ ──────► │   Counter: 5    │
│   Counter → 6   │         │   Counter → 6   │
└─────────────────┘         └─────────────────┘

Replay Attack (BLOCKED):
┌─────────────────┐         ┌─────────────────┐
│   Attacker      │  Code1  │   Car/Garage    │
│   (old code)    │ ──────► │   Counter: 6    │
│                 │         │   REJECTED!     │
└─────────────────┘         └─────────────────┘
```

### The RollJam Attack
```
Step 1: Victim presses button
┌─────────────────┐         ┌─────────────────┐
│   Key Fob       │  Code1  │   JAMMED!       │
│   Counter: 5    │ ──╳───► │   (no signal)   │
│   Counter → 6   │         │                 │
└─────────────────┘         └─────────────────┘
         │
         ▼ Attacker captures Code1

Step 2: Victim presses again (thinks first didn't work)
┌─────────────────┐         ┌─────────────────┐
│   Key Fob       │  Code2  │   Receives      │
│   Counter: 6    │ ──╳───► │   Code1!        │ ← Attacker replays
│   Counter → 7   │         │   Counter → 6   │
└─────────────────┘         └─────────────────┘
         │
         ▼ Attacker captures Code2

Result: Attacker has VALID UNUSED Code2!
┌─────────────────┐         ┌─────────────────┐
│   Attacker      │  Code2  │   Car/Garage    │
│   (fresh code!) │ ──────► │   Counter: 6    │
│                 │         │   ACCEPTED!     │
└─────────────────┘         └─────────────────┘
```

## Features

### 1. RollJam Attack Mode
Full automated attack:
- Arms and waits for victim's signal
- Jams receiver while capturing first code
- Waits for victim's second press
- Replays first code (victim thinks it worked)
- Captures second code (your valid unused code!)
- Ready to use anytime

### 2. Code Capture Mode
Passive capture without jamming:
- Listen for and record signals
- Store multiple codes
- Save to SD card as `.sub` files

### 3. Replay Mode
Transmit captured codes:
- Select from captured codes
- Replay on demand
- Track replay count

### 4. Continuous Jamming
Block signals in target frequency:
- Toggle jamming on/off
- Useful for testing range

### 5. Analyze Captures
View statistics:
- Codes captured
- Signals detected
- File locations

## Usage

### RollJam Attack
1. Select **RollJam Attack** from menu
2. Press **OK** to arm
3. Wait near target (victim's car/garage)
4. When victim presses their remote:
   - App jams + captures Code1
   - Victim thinks it didn't work
5. Victim presses again:
   - App captures Code2, replays Code1
   - Victim's device opens (they leave)
6. **SUCCESS!** You have unused Code2
7. Press **OK** to use Code2 anytime

### Simple Capture
1. Select **Capture Codes**
2. Press **OK** to start listening
3. Captured codes saved to SD card
4. Use **Replay** mode to transmit

## Technical Details

- **Frequency**: 433.92 MHz (configurable)
- **Jam Offset**: +50 kHz (selective jamming)
- **Buffer Size**: 16KB for signal capture
- **Max Codes**: 10 stored in memory
- **Files Saved**: `/ext/subghz/rolljam/`

## Adjusting for Your Target

Edit these defines in `rolljam_app.c`:

```c
#define TARGET_FREQUENCY    433920000   // Common: 433.92, 315, 868 MHz
#define JAM_OFFSET          50000       // Jamming frequency offset
#define RSSI_THRESHOLD      -65.0f      // Signal detection sensitivity
```

Common frequencies:
- **433.92 MHz** - Europe garage doors, many car fobs
- **315 MHz** - US/Japan car fobs
- **868 MHz** - Europe newer systems

## Files

```
rolljam_app/
├── application.fam      # App manifest
├── rolljam_app.c        # Main source (~900 lines)
├── rolljam.png          # App icon
└── images/              # Assets folder
```

## Building

```bash
cd rolljam_app
ufbt
ufbt launch  # To deploy to Flipper
```

## Legal Warning

This tool is for **authorized security research only**. Using it on systems you don't own or have permission to test is **illegal** in most jurisdictions. Always obtain proper authorization before testing.

## References

- Samy Kamkar's RollJam presentation (DEF CON 23)
- Rolling code / KeeLoq analysis papers
- Flipper Zero Sub-GHz documentation
