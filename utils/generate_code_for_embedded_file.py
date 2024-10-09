import argparse
import textwrap
import os
import re
import itertools
import sys
from typing import Iterator


def convert_to_valid_c_identifier(s: str) -> str:
  """Mutate a string to turn it into a valid C identifier.

  In particular, the resultant string is non-empty, only contains '_'
  and alphanumeric characters, and doesn't start with a number.
  """

  has_only_alpha_numeric_characters = re.sub('[^0-9a-zA-Z]+', '_', s)

  starts_with_letter = has_only_alpha_numeric_characters[0:1].isalpha()
  return has_only_alpha_numeric_characters if starts_with_letter else "_" + has_only_alpha_numeric_characters


def source_file_lines_for_embedded_asset(file_bytes: Iterator[bytes], header_file_name: str, array_variable_name: str, array_length_variable_name: str) -> Iterator[str]:
  """Produce the lines of the source code file that embed the given bytes as data.
  """

  COMMA_SEPARATED_HEX_OCTETS_GO_HERE = 'COMMA_SEPARATED_HEX_OCTETS_GO_HERE'

  source_contents = f"""
    #include "{header_file_name}"

    const unsigned char {array_variable_name}[] = {{{COMMA_SEPARATED_HEX_OCTETS_GO_HERE}}};

    const size_t {array_length_variable_name} = sizeof({array_variable_name}) / sizeof({array_variable_name}[0]);
  """

  source_content_start, source_content_end = textwrap.dedent(source_contents).split(COMMA_SEPARATED_HEX_OCTETS_GO_HERE)

  # Convert each byte in the file to a '0x' hex value, followed by a comma
  hex_values = map(lambda b: f'0x{b.hex()},', file_bytes)
  # Chunk the hex values into smallish groups, we'll put one group on each line
  batched_hex_values = itertools.batched(hex_values, 16)
  # Add spaces between the hex values in each chunk and concatenate them into a single string
  lines_of_hex_values = map(lambda batch: ' '.join(batch), batched_hex_values)
  # Add a small indentation to each line
  indented_lines_of_hex_values = map(lambda line: '  ' + line, lines_of_hex_values)

  return itertools.chain(
    source_content_start.split('\n'),
    indented_lines_of_hex_values,
    source_content_end.split('\n')
  )


def header_file_lines_for_embedded_asset(array_variable_name: str, array_length_variable_name: str) -> Iterator[str]:
  """Produce the lines of the header file that embed the given bytes as data.
  """

  header_guard_identifier = f"{array_variable_name.upper()}_H_"

  header_contents = f"""
    #ifndef {header_guard_identifier}
    #define {header_guard_identifier}

    #include <stddef.h>

    extern const unsigned char {array_variable_name}[];
    extern const size_t {array_length_variable_name};

    #endif
  """

  return textwrap.dedent(header_contents).split('\n')


def file_contents_iterator(file):
  """Create an iterator that produces one byte/char of a file at a time"""
  while True:
    chunk = file.read(1)
    if not chunk:
      break
    yield chunk


def generate_code_for_embedded_asset(input_file, destination_folder: str):
  """Generate both a C source file and C header file that contains and references,
  respectively, an embedded copy of the binary data in the given file."""

  input_file_name = os.path.basename(input_file.name)

  header_file_name = f"{input_file_name}.h"
  source_file_name = f"{input_file_name}.c"

  associated_identifier = convert_to_valid_c_identifier(input_file_name)

  array_variable_name = f"{associated_identifier}_data"
  array_length_variable_name = f"{associated_identifier}_length"

  # Generate the header file
  header_file_path = os.path.join(destination_folder, header_file_name)
  with open(header_file_path, 'w') as header_file:

    header_file_lines = header_file_lines_for_embedded_asset(array_variable_name, array_length_variable_name)
    for line in header_file_lines:
      header_file.write(line)
      header_file.write('\n')

  # Generate the source file
  source_file_path = os.path.join(destination_folder, source_file_name)
  with open(source_file_path, 'w') as source_file:

    file_bytes = file_contents_iterator(input_file)
    source_file_lines = source_file_lines_for_embedded_asset(file_bytes, header_file_name, array_variable_name, array_length_variable_name)
    for line in source_file_lines:
      source_file.write(line)
      source_file.write('\n')


if __name__ == "__main__":
  doc_string = """
    Generate a C source file and C header file that support referencing an
    embedded `unsigned char*` that contains exactly the binary contents of a file.

    The generated files are named {x}.h and {x}.c, where {x} is the name of the embedded file.

    The generated source includes two variables:
      (1) An `const unsigned char*` named `{y}_data` that points to the data of the embedded file.
      (2) A `const size_t` named `{y}_length` that contains the length of the data in the embedded file.

    {y} is the name of the embedded file, with non-alphanumeric characters replaced with '_'.
    """

  parser = argparse.ArgumentParser(description=textwrap.dedent(doc_string))

  parser.add_argument('--input', required=True, type=argparse.FileType('rb'), help='The path to a file which will have its data embedded')
  parser.add_argument('--destination-folder', required=True, help='The path to the directory where the C source file and C header file will be written')

  args = parser.parse_args()

  if not os.path.isdir(args.destination_folder):
    print(f"The specified destination directory must exist as a directory, the one provided does not: `{args.destination_folder}`", file=sys.stderr)
  else:
    generate_code_for_embedded_asset(args.input, args.destination_folder)
