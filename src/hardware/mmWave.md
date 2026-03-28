The key idea

For a person sitting at a desk, your strongest signals are usually:

stationary target energy
stationary target distance
target state
sometimes small moving target energy from hand/arm/head motion

So the presence score should be biased toward stationary evidence, not just motion.

A good overall structure is:

presence_confidence = smoothed( zone_score * fused_target_evidence )

where:

zone_score answers: "Is the target where a desk occupant should be?"
fused_target_evidence answers: "Does this frame look like a human is actually there?"
smoothed(...) answers: "Has this evidence been persistent long enough to trust?"
Recommended algorithm
Step 1: Define the expected desk zone

You should treat this as a single-seat detector, not a general room occupancy detector.

Pick a nominal seated distance from the sensor:

desk_distance_m
example: 0.9 m

Then define two tolerances:

inner_radius_m: full confidence region
outer_radius_m: beyond this, confidence becomes 0

Example:

desk_distance_m = 0.9
inner_radius_m = 0.35
outer_radius_m = 0.90

Then define a distance weighting function:

zone_score = 1.0                            if |d - desk_distance| <= inner_radius
zone_score = linearly decay to 0.0         if inner_radius < |d - desk_distance| < outer_radius
zone_score = 0.0                            if |d - desk_distance| >= outer_radius

This is extremely important. Without it, someone walking behind the chair or in the doorway can look like "presence".

Step 2: Normalize moving and stationary energy

Raw radar energies are not yet probabilities. Convert them to [0,1].

For each energy channel, define:

noise_floor
saturation_level

Then:

norm(x) = clamp((x - noise_floor) / (saturation_level - noise_floor), 0, 1)

So:

moving_strength     = norm(moving_target_energy)
stationary_strength = norm(stationary_target_energy)
How to choose those constants

Do this empirically:

collect a few minutes of empty desk data
collect a few minutes of occupied desk data
set:
noise_floor around the upper end of the empty readings
saturation_level around a strong but common occupied reading

This is much better than guessing.

Step 3: Turn target state into a soft evidence term

You said you already have target_state reliably. Good. Use it, but do not let it fully dominate.

Example mapping:

none         -> 0.0
moving only  -> 0.45
stationary   -> 0.75
both         -> 1.0

Why not binary?

Because "moving only" often means someone is entering/leaving or shifting around, which is useful evidence, but for a desk detector, "stationary" is stronger evidence than "moving only".

Step 4: Pick the best distance source

For a desk user, the distance should usually come from:

stationary distance, if valid
otherwise moving distance, if valid
otherwise detection distance, if valid

So:

if stationary target exists:
    d = stationary_target_distance
else if moving target exists:
    d = moving_target_distance
else:
    d = detection_distance

Then feed d into the zone_score.

Step 5: Fuse the evidence into an instantaneous score

Now combine the pieces.

For a desk occupancy detector, I recommend weighting stationary more than moving:

instant_evidence =
    0.50 * stationary_strength +
    0.20 * moving_strength +
    0.20 * state_score +
    0.10 * consistency_score

You may not need a separate consistency score at first, but it can help.

A simple consistency score is:

1.0 if moving/stationary/detection distances agree reasonably well
0.0 if they disagree badly

If you want to keep it simpler, omit it and renormalize weights.

So the simpler version is:

instant_evidence =
    0.60 * stationary_strength +
    0.20 * moving_strength +
    0.20 * state_score

Then apply the desk-zone constraint:

raw_presence = zone_score * instant_evidence

This makes the score high only when both are true:

the radar evidence looks human-like
the target is in the desk region
Better fusion rule than plain weighted sum

A weighted sum is fine, but there is an even nicer option for combining moving and stationary evidence:

combined_motion_stationary = 1 - (1 - moving_component) * (1 - stationary_component)

This acts like a soft OR. If either one is strong, the combined score rises. If both are strong, it rises more.

For a desk detector:

moving_component     = 0.75 * moving_strength + 0.25 * moving_state_flag
stationary_component = 0.85 * stationary_strength + 0.15 * stationary_state_flag

combined_evidence = 1 - (1 - moving_component) * (1 - stationary_component)
raw_presence = zone_score * combined_evidence

I like this formulation a lot because it handles these cases well:

person typing a lot: moving strong
person sitting still: stationary strong
person adjusting in chair: both moderate
empty desk: both low
Step 6: Smooth it over time

This is the part that makes it usable.

If you directly threshold raw_presence, it will flap.

Use an exponential smoother with different attack and release times:

fast attack when someone arrives
slow release when someone appears to leave

That gives you "immediate occupied, reluctant vacant", which is what you want for desk presence.

Use:

alpha = 1 - exp(-dt / tau)

Then:

if raw_presence > confidence:
    alpha = alpha_attack
else:
    alpha = alpha_release

confidence += alpha * (raw_presence - confidence)
Good starting values

At typical sensor update rates:

tau_attack = 0.5 s
tau_release = 8.0 s

This means:

someone sitting down causes the confidence to rise quickly
momentary misses do not instantly drop occupancy

If you want more "sticky occupancy", use:

tau_release = 15 s or even 30 s
Step 7: Use hysteresis, not one threshold

Even with smoothing, use two thresholds:

occupied_threshold = 0.65
vacant_threshold = 0.35

Logic:

if state == VACANT and confidence >= occupied_threshold:
    state = OCCUPIED

if state == OCCUPIED and confidence <= vacant_threshold:
    state = VACANT

This prevents chatter near a single threshold.

Full recommended formula

Here is a practical version I would actually start with.

A. Normalize energies
moving_strength     = clamp((Em - Em_floor) / (Em_sat - Em_floor), 0, 1)
stationary_strength = clamp((Es - Es_floor) / (Es_sat - Es_floor), 0, 1)
B. State flags
moving_flag     = 1 if target_state has moving else 0
stationary_flag = 1 if target_state has stationary else 0
C. Motion and stationary components
moving_component     = 0.75 * moving_strength     + 0.25 * moving_flag
stationary_component = 0.85 * stationary_strength + 0.15 * stationary_flag
D. Fuse them
combined_evidence = 1 - (1 - moving_component) * (1 - stationary_component)
E. Distance zone

Choose best distance d from stationary, else moving, else detection.

Then:

delta = abs(d - desk_distance)

zone_score = 1.0                                            if delta <= inner_radius
zone_score = 1.0 - (delta - inner_radius)/(outer_radius-inner_radius)
                                                           if inner_radius < delta < outer_radius
zone_score = 0.0                                            if delta >= outer_radius
F. Instantaneous raw presence
raw_presence = zone_score * combined_evidence
G. Temporal smoothing
alpha_attack  = 1 - exp(-dt / tau_attack)
alpha_release = 1 - exp(-dt / tau_release)

alpha = alpha_attack  if raw_presence > confidence else alpha_release
confidence = confidence + alpha * (raw_presence - confidence)
H. Final occupancy state
if confidence >= 0.65 -> occupied
if confidence <= 0.35 -> vacant
otherwise hold previous state
Why this works well for a desk

For a desk sensor, the failure modes are usually:

1. Person is present but barely moving

This is why stationary energy gets the biggest weight.

2. Person briefly leans out or radar drops a frame

This is why you need temporal smoothing and slow release.

3. Someone walks behind the chair or nearby

This is why zone scoring matters.

4. Typing or hand movement causes moving target changes

This is good. The fusion formula lets that reinforce occupancy.

Recommended calibration procedure

This matters a lot more than fancy math.

Phase 1: Empty desk

Record for 2 to 5 minutes:

moving energy
stationary energy
distances
target state

From this, estimate:

Em_floor
Es_floor

A good starting method:

use the 95th percentile of empty readings as the floor
Phase 2: Occupied desk

Record for 5 to 10 minutes with:

typing
sitting still
leaning back
reading
minor fidgeting

From this, estimate:

Em_sat
Es_sat

A good starting method:

use something like the 75th to 90th percentile of occupied readings for saturation

This prevents your normalized strengths from spending all their time near 0 or 1.

Practical tuning advice
If it misses a still person

Increase:

stationary weight
release time
stationary sensitivity in the module config
If it thinks someone is present when the desk is empty

Decrease:

outer radius
moving weight
zone width
sensor sensitivity for distant gates
If it flips too often

Increase:

release time
hysteresis gap
If it is too slow to notice arrival

Decrease:

attack time
occupied threshold slightly
C++ style pseudocode
struct PresenceParams {
    float deskDistance = 0.90f;
    float innerRadius = 0.35f;
    float outerRadius = 0.90f;

    float movingFloor = 10.0f;
    float movingSat   = 80.0f;
    float statFloor   = 8.0f;
    float statSat     = 70.0f;

    float tauAttack  = 0.5f;
    float tauRelease = 8.0f;

    float occupiedThreshold = 0.65f;
    float vacantThreshold   = 0.35f;
};

enum class OccupancyState {
    Vacant,
    Occupied
};

struct RadarFrame {
    bool hasMoving;
    bool hasStationary;

    float movingDistance;
    float movingEnergy;

    float stationaryDistance;
    float stationaryEnergy;

    float detectionDistance;
};

class PresenceEstimator {
public:
    explicit PresenceEstimator(PresenceParams p) : params_(p) {}

    float update(const RadarFrame& f, float dtSeconds) {
        float movingStrength = normalize(f.movingEnergy, params_.movingFloor, params_.movingSat);
        float statStrength   = normalize(f.stationaryEnergy, params_.statFloor, params_.statSat);

        float movingComponent = 0.75f * movingStrength + 0.25f * (f.hasMoving ? 1.0f : 0.0f);
        float statComponent   = 0.85f * statStrength   + 0.15f * (f.hasStationary ? 1.0f : 0.0f);

        float combinedEvidence = 1.0f - (1.0f - movingComponent) * (1.0f - statComponent);

        float d = chooseDistance(f);
        float zone = zoneScore(d);

        float rawPresence = zone * combinedEvidence;

        float alphaAttack  = 1.0f - std::exp(-dtSeconds / params_.tauAttack);
        float alphaRelease = 1.0f - std::exp(-dtSeconds / params_.tauRelease);

        float alpha = (rawPresence > confidence_) ? alphaAttack : alphaRelease;
        confidence_ += alpha * (rawPresence - confidence_);

        if (state_ == OccupancyState::Vacant && confidence_ >= params_.occupiedThreshold) {
            state_ = OccupancyState::Occupied;
        } else if (state_ == OccupancyState::Occupied && confidence_ <= params_.vacantThreshold) {
            state_ = OccupancyState::Vacant;
        }

        return confidence_;
    }

    OccupancyState state() const { return state_; }
    float confidence() const { return confidence_; }

private:
    static float clamp01(float x) {
        return std::max(0.0f, std::min(1.0f, x));
    }

    static float normalize(float x, float floor, float sat) {
        if (sat <= floor) return 0.0f;
        return clamp01((x - floor) / (sat - floor));
    }

    float chooseDistance(const RadarFrame& f) const {
        if (f.hasStationary) return f.stationaryDistance;
        if (f.hasMoving)     return f.movingDistance;
        return f.detectionDistance;
    }

    float zoneScore(float d) const {
        float delta = std::fabs(d - params_.deskDistance);

        if (delta <= params_.innerRadius) return 1.0f;
        if (delta >= params_.outerRadius) return 0.0f;

        float t = (delta - params_.innerRadius) / (params_.outerRadius - params_.innerRadius);
        return 1.0f - t;
    }

    PresenceParams params_;
    float confidence_ = 0.0f;
    OccupancyState state_ = OccupancyState::Vacant;
};
One improvement I strongly recommend

Add a short-lived linger floor when stationary evidence was recently high.

Example idea:

if stationary component exceeded 0.7 in the last few seconds
do not allow raw_presence to instantly collapse to zero
instead floor it at 0.15 or 0.20 briefly

This helps with cases where the sensor momentarily loses a very still person.

Conceptually:

if (recentStrongStationaryWithin3s) {
    rawPresence = std::max(rawPresence, 0.20f);
}

That said, I would first try the attack/release smoother and hysteresis before adding this.

My recommended starting values

If you want a first-pass setup, I would begin with:

deskDistance      = measured seat distance
innerRadius       = 0.30 to 0.40 m
outerRadius       = 0.80 to 1.00 m

weights:
stationary heavier than moving

tauAttack         = 0.5 s
tauRelease        = 10 s

occupiedThreshold = 0.65
vacantThreshold   = 0.35
If you want the simplest possible version

If you want a very compact first implementation:

moving_strength     = normalized moving energy
stationary_strength = normalized stationary energy
state_score         = 0 / 0.45 / 0.75 / 1.0 from target state
zone_score          = desk distance weighting

raw = zone_score * (0.6 * stationary_strength + 0.2 * moving_strength + 0.2 * state_score)
confidence = asymmetric low-pass filter(raw)
occupied if confidence > 0.65
vacant if confidence < 0.35

That simple version will probably already work decently.