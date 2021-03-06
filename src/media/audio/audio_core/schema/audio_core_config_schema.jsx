{
  "definitions": {
    "volume_mapping": {
      "type": "object",
      "properties": {
        "level" : "number",
        "db": "number"
      },
      "required": ["level", "db"],
      "additionalProperties": false
    },
    "output_stream_type": {
      "enum": [
        "background", "communications", "interruption", "media", "system_agent"
      ]
    },
    "effect": {
      "type": "object",
      "properties": {
        "lib": "string",
        "name": "string",
        "config": {},
        "_comment": "string"
      },
      "required": [ "lib" ],
      "additionalProperties": false
    },
    "mix_group": {
      "type": "object",
      "properties": {
        "name": "string",
        "_comment": "string",
        "streams": {
          "type": "array",
          "items": { "$ref": "#/definitions/output_stream_type" }
        },
        "effects": {
          "type": "array",
          "items": { "$ref": "#/definitions/effect" }
        }
      },
      "additionalProperties": false
    }
   },
  "type": "object",
  "properties": {
    "volume_curve": {
      "type": "array",
      "items": { "$ref": "#/definitions/volume_mapping" }
    },
    "pipeline": {
      "type": "object",
      "properties": {
        "_comment": "string",
        "name": "string",
        "output_streams": {
          "type": "array",
          "items": { "$ref": "#/definitions/mix_group" }
        },
        "mix": {
          "$ref": "#/definitions/mix_group"
        },
        "linearize": {
          "$ref": "#/definitions/mix_group"
        }
      },
      "additionalProperties": false
    }
  },
  "required": ["volume_curve"],
  "additionalProperties": false
}
