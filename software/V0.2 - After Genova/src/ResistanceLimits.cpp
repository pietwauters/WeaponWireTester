

// Body Wire tests:
// Limit for each individual wire = 1 Ohm
constexpr float Max_SingleBodyWire_Ohm = 1.0;
int Max_SingleBodyWireax_ADC;

// Below variables store the bodywire resistances, such that they can be used
// To compensate for in the other modes

float CentralWire_Ohm;
float Wire_15mm_Ohm;
float Wire_20mm_Ohm;

// ------------------------------------------------------------------------------

// Epee tests
// Round Trip -> 2 Ohm (so 1 Ohm per wire)
constexpr float Max_SingleEpeeWire_Ohm = 1.0;
constexpr float Max_TipResistance_Ohm = 2.0;   // To be checked i this is with or without the wire(s) in the weapon
constexpr float Max_GuardResistance_Ohm = 1.0;
// Below values need to be updated at runtime with the bodywire values

int Max_Epee_CentralWireLimit_ADC;
int Max_Epee_15mmWire_Limit_ADC;
int Max_Epee_15mmWireToTip_ADC;

// ------------------------------------------------------------------------------

// Lamé tests

constexpr float Max_Lame_Ohm = 5.0;