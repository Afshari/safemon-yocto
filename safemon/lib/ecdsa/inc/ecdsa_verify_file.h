#pragma once
#include <string>

namespace ecdsa {

// Verify that <file_path> matches the signature in <file_path>.sig
// using the public key loaded from <pub_key_path> (JSON, little-endian limbs).
// Returns true if valid, false otherwise.
bool verify_file(const std::string& file_path,
                 const std::string& pub_key_path);

} // namespace ecdsa