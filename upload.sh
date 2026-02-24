# Create the release

download_license_with_commit() {
    local repo="$1"                  # e.g., CTSRD-CHERI/cheribuild
    local commit="$2"                # optional commit string (can be empty)
    local branch="${3:-master}"      # optional branch, default "master"
    local path="${4:-LICENSE}"       # optional file path, default "LICENSE"
    local outfile="${5:-LICENSE-${repo##*/}}" # default output file

    # Build raw.githubusercontent URL
    local url="https://raw.githubusercontent.com/$repo/$branch/$path"

    # Download the file
    curl -L "$url" -o "$outfile"

    # Append provenance information
    {
        echo
        echo "----"
        echo "Source: https://github.com/$repo"
        echo "Branch: $branch"
        echo "Commit: ${commit:-HEAD}"
        echo "Retrieved: $(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    } >> "$outfile"
}

download_license_with_commit CTSRD-CHERI/cheribuild df913531
download_license_with_commit CHERI-Alliance/qemu 2a2e882b
download_license_with_commit CHERI-Alliance/llvm-project 1ca584e7 codasip-cheri-riscv LICENSE.TXT
download_license_with_commit CHERI-Alliance/gdb df929d4d codasip-cheri-riscv COPYING3

version="0.2"
gh release create custom-llvm-debs-${version} \
  ./cheribuild_0.${version}_amd64.deb \
  ./codasip-gdb_0.${version}_amd64.deb \
  ./codasip-llvm_0.${version}_amd64.deb \
  ./codasip-qemu_0.${version}_amd64.deb \
  ./codasip-qemu-patched_0.${version}_amd64.deb \
  ./LICENSE-cheribuild \
  ./LICENSE-qemu \
  ./LICENSE-llvm \
  ./LICENSE-gdb \
  --repo cheri-zephyr-project/custom-debs \
  --title "custom-llvm-debs-${version}" \
  --notes "custom-llvm-debs-${version}"
