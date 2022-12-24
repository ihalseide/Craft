#ifndef _Physics_h
#define _Physics_h


// Physiscs settings
typedef struct {
    float grav;                   // gravity
    float walksp;                 // walking speed
    float flysp;                  // flying speed
    float jumpaccel;              // jump vertical power or acceleration
    float flyr;                   // flying movement resistance
    float groundr;                // ground horizontal resistance factor
    float airhr;                  // air horizontal resistance factor
    float airvr;                  // air vertical resistance factor
    float jumpcool;               // jump cool-down time
    float blockcool;              // block placing cool-down time
    float dblockcool;             // block destroying cool-down time
    float min_impulse_damage;     // minimum velocity change required to take damage
    float impulse_damage_min;     // base damage dealt with velocity change exceeding the limit above
    float impulse_damage_scale;   // damage multiplier for velocity change exceeding the limit above
} PhysicsConfig;


#endif
