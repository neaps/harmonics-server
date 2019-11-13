## Harmonic Generator Web Service

A web service to generate tidal hamonic constituents using LibCongen, Harmgen from [XTide](https://flaterco.com/xtide).

### Changes made to harmgen

Harmgen only generates SQL queries for XTide's own Harmonics database. This repository has a patched version of Harmgen that instead outputs data in JSON format.

## API endpoints

### Generate harmonics file

**POST** `/api/v1/generate`

Accepts a JSON object with:

- `uuid` - The unique ID of the request. This is used to check on the status of parsing later.
- `levels` - An array of water levels, each level is an object with `t`, the **ISO-formatted time** of water level in UTC, and `v`, the observed water level **in meters**.

For example:

```json
{
  "uuid": "101010101000",
  "levels": [
    { "t": "2018-01-02T06:00:00.000Z", "v": -1.4 },

    { "t": "2018-01-02T07:00:00.000Z", "v": -1.23 }
  ]
}
```

### Check on status of processing

**GET** `/api/v1/status/[id]`

Returns a simple object indicating whether the processing is complete or not.

```json
{
  "done": true
}
```

### Get tidal constituents

**GET** `/api/v1/get/[id]`

Returns a JSON object of tidal constituents that were derived from the `generate` API call. Results look like:

```json
{
  "SA": { "amplitude": 0.078, "phase": 210.05 },
  "SSA": { "amplitude": 0.0089, "phase": 177.17 }
}
```
