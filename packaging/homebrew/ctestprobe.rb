# Homebrew formula for ctestprobe.
#
# This lives in the ctestprobe repo as a template — Homebrew formulae
# must live in a tap repository named homebrew-<something>. To publish:
#
#   1. Create a new empty repo at github.com/ocrosby/homebrew-ctestprobe
#   2. Copy this file to Formula/ctestprobe.rb in that repo
#   3. Bump `url`, `sha256`, and `version` after each ctestprobe release
#
# The sha256 line below is a placeholder; regenerate with:
#
#   curl -fsSL https://github.com/ocrosby/ctestprobe/releases/download/vX.Y.Z/ctestprobe-X.Y.Z.tar.gz | shasum -a 256

class Ctestprobe < Formula
  desc "Minimal unit-testing framework for C"
  homepage "https://github.com/ocrosby/ctestprobe"
  url "https://github.com/ocrosby/ctestprobe/releases/download/v2.0.0/ctestprobe-2.0.0.tar.gz"
  sha256 "REPLACE_ME_WITH_THE_ACTUAL_SHA256"
  license "GPL-3.0-only"
  head "https://github.com/ocrosby/ctestprobe.git", branch: "main"

  depends_on "pkg-config" => :test

  def install
    system "make", "PREFIX=#{prefix}", "install"
  end

  test do
    (testpath/"smoke.c").write <<~C
      #include <ctestprobe.h>
      static void t_ok(void) { CTP_ASSERT_EQ_INT(1 + 1, 2); }
      int main(int argc, char **argv) {
          ctestprobe_init();
          ctestprobe_register("ok", t_ok);
          return ctestprobe_main(argc, argv);
      }
    C
    system ENV.cc, "smoke.c", "-I#{include}", "-L#{lib}", "-lctestprobe",
                    "-o", "smoke"
    assert_match "1 passed, 0 failed", shell_output("./smoke")
  end
end
