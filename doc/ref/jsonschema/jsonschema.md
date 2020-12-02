### jsonschema extension

The jsonschema extension implements the JSON Schema [Draft 7](https://json-schema.org/specification-links.html#draft-7) specification for validating input JSON. (since 0.160.0)

### Compliance level

The jsonschema extension supports JSON Schema draft 7.

The validator understands the following [format types](https://json-schema.org/understanding-json-schema/reference/string.html#format):

|                      | Draft 7            |
|----------------------|--------------------|
| date                 | :heavy_check_mark: |
| time                 | :heavy_check_mark: |
| date-time            | :heavy_check_mark: |
| email                | :heavy_check_mark: |
| hostname             | :heavy_check_mark: |
| ipv4                 | :heavy_check_mark: |
| ipv6                 | :heavy_check_mark: |
| regex                | :heavy_check_mark: |

Any other format type is ignored.

### Classes
<table border="0">
  <tr>
    <td><a href="json_validator.md">json_validator</a></td>
    <td>JSON Schema validator.</td> 
  </tr>
</table>

### Functions

<table border="0">
  <tr>
    <td><a href="make_schema.md">make_schema</a></td>
    <td>Loads a JSON Schema and returns a <code>shared_ptr</code> to a <code>json_schema</code>. 
  </tr>
</table>
  
### Default values
  
The JSON Schema Specification includes the ["default" keyword](https://json-schema.org/understanding-json-schema/reference/generic.html)  
for specifying a default value, but doesn't prescribe how implementations should use it during validation.
Some implementations ignore the default keyword, others support updating the input JSON to fill in a default value 
for a missing key/value pair. This implementation follows the approach of
[JSON schema validator for JSON for Modern C++ ](https://github.com/pboettch/json-schema-validator),
which returns a JSONPatch document that may be further applied to the input JSON to add the
missing key/value pairs.
  
### Examples

The examples below are from [JSON Schema Miscellaneous Examples](https://json-schema.org/learn/miscellaneous-examples.html)
and the [JSON Schema Test Suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite).

#### Arrays of things

```c++
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>

// for brevity
using jsoncons::json;
namespace jsonschema = jsoncons::jsonschema; 

int main() 
{
    // JSON Schema
    json schema = json::parse(R"(
{
  "$id": "https://example.com/arrays.schema.json",
  "$schema": "http://json-schema.org/draft-07/schema#",
  "description": "A representation of a person, company, organization, or place",
  "type": "object",
  "properties": {
    "fruits": {
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "vegetables": {
      "type": "array",
      "items": { "$ref": "#/definitions/veggie" }
    }
  },
  "definitions": {
    "veggie": {
      "type": "object",
      "required": [ "veggieName", "veggieLike" ],
      "properties": {
        "veggieName": {
          "type": "string",
          "description": "The name of the vegetable."
        },
        "veggieLike": {
          "type": "boolean",
          "description": "Do I like this vegetable?"
        }
      }
    }
  }
}
    )");

    // Data
    json data = json::parse(R"(
{
  "fruits": [ "apple", "orange", "pear" ],
  "vegetables": [
    {
      "veggieName": "potato",
      "veggieLike": true
    },
    {
      "veggieName": "broccoli",
      "veggieLike": "false"
    },
    {
      "veggieName": "carrot",
      "veggieLike": false
    },
    {
      "veggieName": "Swiss Chard"
    }
  ]
}
   )");

    // Will throw schema_error if JSON Schema loading fails
    auto schema_doc = jsonschema::make_schema(schema);

    std::size_t error_count = 0;
    auto reporter = [&error_count](const jsonschema::validation_error& e)
    {
        ++error_count;
        std::cout << e.what() << "\n";
    };

    jsonschema::json_validator<json> validator(schema_doc);

    // Will call reporter for each schema violation
    validator.validate(data, reporter);

    std::cout << "\nError count: " << error_count << "\n\n";
}
```

Output:
```
/vegetables/1/veggieLike: Expected boolean, found string
/vegetables/3: Required key "veggieLike" not found

Error count: 2
```

#### Using a URIResolver to resolve references to schemas defined in external files

In this example, the main schema defines a reference using
the `$ref` property to a second schema defined in the file `name.json`,
shown below:

```json
{
    "definitions": {
        "orNull": {
            "anyOf": [
                {
                    "type": "null"
                },
                {
                    "$ref": "#"
                }
            ]
        }
    },
    "type": "string"
}
```
You need to provide a `URIResolver` to tell jsoncons how to resolve the reference into a 
JSON Schema document.

```c++
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <fstream>

// for brevity
using jsoncons::json;
namespace jsonschema = jsoncons::jsonschema; 

json resolver(const jsoncons::uri& uri)
{
    std::cout << "uri: " << uri.string() << ", path: " << uri.path() << "\n\n";

    std::string pathname = /* path_to_directory */;
    pathname += std::string(uri.path());

    std::fstream is(pathname.c_str());
    if (!is)
        throw jsonschema::schema_error("Could not open " + std::string(uri.base()) + " for schema loading\n");

    return json::parse(is);        
}

int main()
{ 
    // JSON Schema
    json schema = json::parse(R"(
{
    "$id": "http://localhost:1234/object",
    "type": "object",
    "properties": {
        "name": {"$ref": "name.json#/definitions/orNull"}
    }
}
    )");

    // Data
    json data = json::parse(R"(
{
    "name": {
        "name": null
    }
}
    )");

    // Will throw schema_error if JSON Schema loading fails
    auto schema_doc = jsonschema::make_schema(schema, resolver);

    std::size_t error_count = 0;
    auto reporter = [&error_count](const jsonschema::validation_error& e)
    {
        ++error_count;
        std::cout << e.what() << "\n";
    };

    jsonschema::json_validator<json> validator(schema_doc);

    // Will call reporter for each schema violation
    validator.validate(data, reporter);

    std::cout << "\nError count: " << error_count << "\n\n";
}
```
Output:
```
uri: http://localhost:1234/name.json, path: /name.json

/name: No subschema matched, but one of them is required to match

Error count: 1
```

#### Default values

```c++
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <jsoncons_ext/jsonpatch/jsonpatch.hpp>
#include <fstream>

int main() 
{
    // JSON Schema
    json schema = json::parse(R"(
{
    "properties": {
        "bar": {
            "type": "string",
            "minLength": 4,
            "default": "bad"
        }
    }
}
    )");

    // Data
    json data = json::parse("{}");

    // will throw schema_error if JSON Schema loading fails 
    auto schema_doc = jsonschema::make_schema(schema, resolver); 

    jsonschema::json_validator<json> validator(schema_doc); 

    // will throw validation_error on first encountered schema violation 
    json patch = validator.validate(data); 

    std::cout << "Patch: " << patch << "\n";

    std::cout << "Original data: " << data << "\n";

    jsonpatch::apply_patch(data, patch);

    std::cout << "Patched data: " << data << "\n\n";
}
```
Output:
```
Patch: [{"op":"add","path":"/bar","value":"bad"}]
Original data: {}
Patched data: {"bar":"bad"}
```