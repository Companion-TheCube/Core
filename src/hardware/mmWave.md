mmWave presence detection now uses a rolling-window classifier derived from
`mmWave Sensor Graphing/algo.md`.

Current behavior

- The estimator keeps a timestamped sample buffer sized to the largest active
  averaging window.
- Detection distance, moving distance, stationary distance, and stationary
  energy each use their own configurable averaging window from `GlobalSettings`.
- Presence is classified from the latest `targetState`:
  - `0`: detection distance only
  - `1`: detection + moving distance
  - `2`: detection + stationary distance
  - `3`: detection + moving + stationary distance
- The state machine is:
  - `Present` if every valid average is below `100 cm`
  - `Absent` if every valid average is above its adjusted absent threshold
  - otherwise hold the previous state

Absent thresholds

- Base thresholds are:
  - detection `150 cm`
  - moving `200 cm`
  - stationary `200 cm`
- If the latest `targetState == 0`, reduce thresholds by `15%`.
- If average stationary energy is below `75`, reduce thresholds by another `10%`.
- The combined multiplier is floored at `0.5`.

Notes

- Inactive distance channels are intentionally ignored because the sensor may
  leave stale values in those fields.
- Sensor tuning and calibration flows have been removed. The only user-facing
  controls are the four averaging windows exposed in the Sensors menu.
