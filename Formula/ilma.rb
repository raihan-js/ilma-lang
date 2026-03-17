class Ilma < Formula
  desc "A programming language for children. Simpler than Python."
  homepage "https://ilma-lang.dev"
  url "https://github.com/raihan-js/ilma-lang/archive/refs/tags/v0.7.0.tar.gz"
  sha256 "FILL_AFTER_RELEASE"
  license "MIT"
  version "0.7.0"
  head "https://github.com/raihan-js/ilma-lang.git", branch: "main"

  depends_on "gcc" => :build

  def install
    system "make", "all"
    system "make", "lsp"
    bin.install "build/ilma"
    bin.install "build/ilma-lsp"
    (lib/"ilma/runtime").mkpath
    (lib/"ilma/runtime/modules").mkpath
    install_files = Dir["src/runtime/ilma_runtime.c", "src/runtime/ilma_runtime.h"]
    install_files.each { |f| (lib/"ilma/runtime").install f }
    Dir["src/runtime/modules/*.c", "src/runtime/modules/*.h"].each do |f|
      (lib/"ilma/runtime/modules").install f
    end
    (share/"ilma/packages").mkpath
    Dir["packages/*/"].each do |pkg|
      (share/"ilma/packages").install pkg
    end
  end

  def post_install
    (var/"ilma/packages").mkpath
  end

  def caveats
    <<~EOS
      To get started:
        echo 'say "Bismillah"' > hello.ilma
        ilma hello.ilma

      Install packages:
        ilma get math
        ilma packages --available

      Documentation:
        https://ilma-lang.dev/docs.html
    EOS
  end

  test do
    (testpath/"hello.ilma").write('say "Bismillah"')
    assert_match "Bismillah", shell_output("#{bin}/ilma #{testpath}/hello.ilma")
    assert_match "0.7.0", shell_output("#{bin}/ilma --version")
  end
end
