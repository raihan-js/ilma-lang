/* ═══════════════════════════════════════════════════════
   ILMA Website — main.js
   Shared JS for all pages
   ═══════════════════════════════════════════════════════ */

(function () {
  'use strict';

  /* ── Sticky Nav ──────────────────────────────────────── */
  function initStickyNav() {
    var nav = document.querySelector('.site-nav');
    if (!nav) return;
    function onScroll() {
      nav.classList.toggle('scrolled', window.scrollY > 12);
    }
    window.addEventListener('scroll', onScroll, { passive: true });
    onScroll();
  }

  /* ── Active Nav Link ─────────────────────────────────── */
  function initActiveNavLink() {
    var links = document.querySelectorAll('.nav-links a, .mobile-menu a');
    var current = window.location.pathname.replace(/\/$/, '') || '/';
    links.forEach(function (link) {
      var href = link.getAttribute('href');
      if (!href) return;
      var linkPath = href.replace(/\/$/, '') || '/';
      if (linkPath === current ||
          (linkPath !== '/' && linkPath.length > 1 && current.startsWith(linkPath))) {
        link.classList.add('active');
      }
    });
  }

  /* ── Mobile Hamburger ────────────────────────────────── */
  function initMobileMenu() {
    var hamburger = document.querySelector('.nav-hamburger');
    var mobileMenu = document.querySelector('.mobile-menu');
    if (!hamburger || !mobileMenu) return;

    hamburger.addEventListener('click', function () {
      var isOpen = mobileMenu.classList.toggle('open');
      hamburger.classList.toggle('open', isOpen);
      document.body.style.overflow = isOpen ? 'hidden' : '';
    });

    mobileMenu.querySelectorAll('a').forEach(function (link) {
      link.addEventListener('click', function () {
        mobileMenu.classList.remove('open');
        hamburger.classList.remove('open');
        document.body.style.overflow = '';
      });
    });

    document.addEventListener('click', function (e) {
      if (!hamburger.contains(e.target) && !mobileMenu.contains(e.target)) {
        mobileMenu.classList.remove('open');
        hamburger.classList.remove('open');
        document.body.style.overflow = '';
      }
    });
  }

  /* ── Copy to Clipboard ───────────────────────────────── */
  function copyText(text, btn) {
    if (navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(text).then(function () {
        showCopied(btn);
      }).catch(function () { fallbackCopy(text, btn); });
    } else {
      fallbackCopy(text, btn);
    }
  }

  function fallbackCopy(text, btn) {
    var ta = document.createElement('textarea');
    ta.value = text;
    ta.style.cssText = 'position:fixed;top:-9999px;left:-9999px;opacity:0;';
    document.body.appendChild(ta);
    ta.select();
    try { document.execCommand('copy'); showCopied(btn); } catch (e) {}
    document.body.removeChild(ta);
  }

  function showCopied(btn) {
    if (!btn) return;
    var orig = btn.innerHTML;
    btn.innerHTML = '&#10003; Copied!';
    btn.classList.add('copied');
    setTimeout(function () {
      btn.innerHTML = orig;
      btn.classList.remove('copied');
    }, 2000);
  }

  function initCopyButtons() {
    document.addEventListener('click', function (e) {
      var btn = e.target.closest('[data-copy]');
      if (!btn) return;
      var text = btn.dataset.copy;
      if (!text) {
        var pre = btn.closest('.code-block, .code-window');
        if (pre) {
          var codeEl = pre.querySelector('pre');
          text = codeEl ? codeEl.textContent : '';
        }
      }
      if (text) copyText(text, btn);
    });
  }

  /* ── Tab Component ───────────────────────────────────── */
  function initTabs() {
    // data-tab-group based tabs
    document.querySelectorAll('[data-tab-group]').forEach(function (group) {
      var buttons = group.querySelectorAll('[data-tab]');
      var contentId = group.dataset.tabGroup;
      var contents = document.querySelectorAll('[data-tab-content="' + contentId + '"]');
      buttons.forEach(function (btn) {
        btn.addEventListener('click', function () {
          var target = btn.dataset.tab;
          buttons.forEach(function (b) { b.classList.remove('active'); });
          btn.classList.add('active');
          contents.forEach(function (c) {
            c.classList.toggle('active', c.dataset.tabId === target);
          });
        });
      });
    });

    // .tabs / .tab-btn based
    document.querySelectorAll('.tabs').forEach(function (tabsEl) {
      var btns = tabsEl.querySelectorAll('.tab-btn');
      btns.forEach(function (btn) {
        btn.addEventListener('click', function () {
          var parent = tabsEl.closest('.tabs-wrapper') || tabsEl.parentElement;
          var target = btn.dataset.tab;
          btns.forEach(function (b) { b.classList.remove('active'); });
          btn.classList.add('active');
          if (parent) {
            parent.querySelectorAll('.tab-content').forEach(function (c) {
              c.classList.toggle('active', c.dataset.tabContent === target);
            });
          }
        });
      });
    });
  }

  /* ── Showcase Tabs ───────────────────────────────────── */
  function initShowcaseTabs() {
    document.querySelectorAll('.showcase-tab-btn').forEach(function (btn) {
      btn.addEventListener('click', function () {
        var wrap = btn.closest('.showcase-tabs-container');
        if (!wrap) return;
        var target = btn.dataset.showcase;
        wrap.querySelectorAll('.showcase-tab-btn').forEach(function (b) {
          b.classList.remove('active');
        });
        btn.classList.add('active');
        var bodyWrap = btn.closest('.showcase-grid') || document.body;
        bodyWrap.querySelectorAll('.showcase-tab-body').forEach(function (c) {
          c.classList.toggle('active', c.dataset.showcaseContent === target);
        });
      });
    });
  }

  /* ── OS Tab Auto-detect ──────────────────────────────── */
  function initOSTabs() {
    var tabs = document.querySelectorAll('.install-os-btn');
    if (!tabs.length) return;
    var p = navigator.platform || '';
    var ua = navigator.userAgent || '';
    var detected = 'linux';
    if (/Win/i.test(p) || /Windows/i.test(ua)) detected = 'windows';
    else if (/Mac/i.test(p) || /Mac/i.test(ua)) detected = 'macos';
    tabs.forEach(function (btn) {
      if (btn.dataset.os === detected) btn.click();
    });
  }

  /* ── Scroll Animations ───────────────────────────────── */
  function initScrollAnimations() {
    var items = document.querySelectorAll('.animate-fade-up');
    if (!items.length) return;
    if (!('IntersectionObserver' in window)) {
      items.forEach(function (el) { el.classList.add('visible'); });
      return;
    }
    var observer = new IntersectionObserver(function (entries) {
      entries.forEach(function (entry) {
        if (entry.isIntersecting) {
          entry.target.classList.add('visible');
          observer.unobserve(entry.target);
        }
      });
    }, { threshold: 0.1 });
    items.forEach(function (el) { observer.observe(el); });
  }

  /* ── ILMA Syntax Highlighter ─────────────────────────── */
  function escapeHtml(text) {
    return text
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
  }

  var ILMA_KEYWORDS_RE = /\b(remember|say|ask|for each|give back|recipe|blueprint|comes from|keep going while|otherwise if|otherwise|when wrong|if|repeat|while|check|when|shout|use|try|bag|notebook|me|in|and|or|not|yes|no|empty|is not|is)\b/g;

  function highlightIlmaLine(line) {
    if (/^\s*#/.test(line)) {
      return '<span class="syn-comment">' + escapeHtml(line) + '</span>';
    }
    var tokens = [];
    var remaining = line;
    var result = '';

    // We'll do a simple pass: strings first, then rest
    var parts = remaining.split(/(\"[^\"]*\"|'[^']*')/g);
    parts.forEach(function (part, idx) {
      if (idx % 2 === 1) {
        // it's a string literal
        result += '<span class="syn-string">' + escapeHtml(part) + '</span>';
      } else {
        // process non-string part
        var escaped = escapeHtml(part);
        // numbers
        escaped = escaped.replace(/\b(\d+(?:\.\d+)?)\b/g, '<span class="syn-number">$1</span>');
        // keywords
        escaped = escaped.replace(ILMA_KEYWORDS_RE, '<span class="syn-keyword">$1</span>');
        // function calls (word before open paren, not keywords)
        escaped = escaped.replace(/\b([a-zA-Z_]\w*)\s*(?=\()/g, function (m, name) {
          return '<span class="syn-func">' + name + '</span>';
        });
        // operators
        escaped = escaped.replace(/(!=|==|&lt;=|&gt;=|&lt;|&gt;|(?<![=!<>])=(?![=]))/g,
          '<span class="syn-op">$1</span>');
        result += escaped;
      }
    });
    return result;
  }

  function highlightIlma(code) {
    return code.split('\n').map(highlightIlmaLine).join('\n');
  }

  function initSyntaxHighlighting() {
    document.querySelectorAll('code.lang-ilma, code.ilma').forEach(function (el) {
      el.innerHTML = highlightIlma(el.textContent);
    });
    document.querySelectorAll('.ilma-code').forEach(function (el) {
      el.innerHTML = highlightIlma(el.textContent);
    });
  }

  /* ── Accordion ───────────────────────────────────────── */
  function initAccordion() {
    document.querySelectorAll('.accordion-btn').forEach(function (btn) {
      btn.addEventListener('click', function () {
        var body = btn.nextElementSibling;
        if (!body) return;
        var isOpen = btn.classList.toggle('open');
        body.classList.toggle('open', isOpen);
      });
    });
  }

  /* ── Docs Sidebar Active Tracking ───────────────────── */
  function initDocsSidebarTracking() {
    var sidebar = document.querySelector('.docs-sidebar');
    if (!sidebar) return;
    var headings = document.querySelectorAll('.docs-content h2[id], .docs-content h3[id]');
    if (!headings.length) return;
    var links = sidebar.querySelectorAll('a[href^="#"]');
    if (!('IntersectionObserver' in window)) return;

    var observer = new IntersectionObserver(function (entries) {
      entries.forEach(function (entry) {
        if (entry.isIntersecting) {
          var id = entry.target.id;
          links.forEach(function (link) {
            link.classList.toggle('active', link.getAttribute('href').slice(1) === id);
          });
        }
      });
    }, { rootMargin: '-20px 0px -70% 0px', threshold: 0 });

    headings.forEach(function (h) { observer.observe(h); });
  }

  /* ── Docs Mobile Toggle ──────────────────────────────── */
  function initDocsMobileToggle() {
    var toggle = document.querySelector('.docs-sidebar-toggle');
    var sidebar = document.querySelector('.docs-sidebar');
    if (!toggle || !sidebar) return;
    toggle.addEventListener('click', function () {
      sidebar.classList.toggle('open');
    });
    sidebar.querySelectorAll('a').forEach(function (link) {
      link.addEventListener('click', function () {
        if (window.innerWidth <= 768) sidebar.classList.remove('open');
      });
    });
  }

  /* ── Share URL ───────────────────────────────────────── */
  function initShareUrl() {
    var shareBtn = document.querySelector('[data-share-url]');
    if (!shareBtn) return;
    shareBtn.addEventListener('click', function () {
      copyText(window.location.href, shareBtn);
    });
  }

  /* ── Search Filter ───────────────────────────────────── */
  function initSearch() {
    var searchInput = document.querySelector('[data-search-input]');
    var searchGrid  = document.querySelector('[data-search-grid]');
    if (!searchInput || !searchGrid) return;
    searchInput.addEventListener('input', function () {
      var q = searchInput.value.toLowerCase().trim();
      searchGrid.querySelectorAll('[data-search-item]').forEach(function (card) {
        card.style.display = (!q || card.textContent.toLowerCase().includes(q)) ? '' : 'none';
      });
    });
  }

  /* ── Package Registry Loader ─────────────────────────── */
  function initPackageRegistry() {
    var grid = document.querySelector('[data-packages-grid]');
    if (!grid) return;

    var FALLBACK = [
      { name: 'math-tools',  version: '1.2.0', description: 'Extended maths helpers: trigonometry, statistics, matrix ops.', tags: ['maths','science'] },
      { name: 'ilma-colors', version: '0.8.3', description: 'ANSI terminal colour printing and styled output helpers.',       tags: ['terminal','ui'] },
      { name: 'quran-data',  version: '2.0.1', description: 'Surah/Ayah lookup, transliteration and translation utilities.',  tags: ['islamic','data'] },
      { name: 'http-simple', version: '0.5.0', description: 'Minimal HTTP client for fetching APIs from scripts.',            tags: ['web','network'] },
      { name: 'json-tools',  version: '1.0.4', description: 'Parse and stringify JSON from within ILMA scripts.',            tags: ['data','parsing'] },
      { name: 'csv-reader',  version: '0.3.2', description: 'Read and iterate CSV files as bags of notebooks.',              tags: ['data','files'] },
      { name: 'date-time',   version: '1.1.0', description: 'Date arithmetic, formatting and Islamic Hijri calendar.',       tags: ['date','time'] },
      { name: 'file-utils',  version: '0.6.1', description: 'Read, write, list and move files with clean syntax.',           tags: ['files','io'] },
      { name: 'geometry',    version: '0.4.0', description: 'Area, perimeter, volume formulas for common shapes.',           tags: ['maths','geometry'] },
      { name: 'arabic-text', version: '1.3.0', description: 'Arabic text normalisation, shaping and Tashkeel utilities.',   tags: ['arabic','text'] }
    ];

    function renderPackages(pkgs) {
      grid.innerHTML = pkgs.map(function (pkg) {
        var tags = (pkg.tags || []).map(function (t) {
          return '<span class="package-tag">' + escapeHtml(t) + '</span>';
        }).join('');
        var cmd = 'ilma get ' + pkg.name;
        return '<div class="package-card animate-fade-up" data-search-item>' +
          '<div class="package-card-header">' +
          '<span class="package-name">' + escapeHtml(pkg.name) + '</span>' +
          '<span class="package-version">v' + escapeHtml(pkg.version) + '</span>' +
          '</div>' +
          '<p class="package-desc">' + escapeHtml(pkg.description) + '</p>' +
          '<div class="package-tags">' + tags + '</div>' +
          '<div class="package-copy-row">' +
          '<span class="package-copy-cmd">' + escapeHtml(cmd) + '</span>' +
          '<button class="copy-btn" data-copy="' + escapeHtml(cmd) + '" title="Copy install command">Copy</button>' +
          '</div></div>';
      }).join('');
      initScrollAnimations();
    }

    function showSkeletons() {
      grid.innerHTML = Array(6).fill(
        '<div class="package-card">' +
        '<div class="skeleton" style="height:18px;width:55%;margin-bottom:12px;border-radius:4px;"></div>' +
        '<div class="skeleton" style="height:13px;width:88%;margin-bottom:6px;border-radius:4px;"></div>' +
        '<div class="skeleton" style="height:13px;width:70%;border-radius:4px;"></div>' +
        '</div>'
      ).join('');
    }

    showSkeletons();
    fetch('/packages/registry.json')
      .then(function (r) { return r.ok ? r.json() : Promise.reject(); })
      .then(function (data) { renderPackages(Array.isArray(data) ? data : FALLBACK); })
      .catch(function () { renderPackages(FALLBACK); });
  }

  /* ── Learn Progress ──────────────────────────────────── */
  function initLearnProgress() {
    var bar = document.querySelector('.progress-bar-fill');
    if (!bar) return;
    try {
      var done = JSON.parse(localStorage.getItem('ilma_lessons_done') || '[]');
      var cards = document.querySelectorAll('.lesson-card');
      if (!cards.length) return;
      bar.style.width = Math.round((done.length / cards.length) * 100) + '%';
      cards.forEach(function (card) {
        var id = card.dataset.lessonId;
        if (id && done.includes(id)) {
          var footer = card.querySelector('.lesson-card-footer');
          if (footer && !footer.querySelector('.lesson-completed')) {
            var badge = document.createElement('span');
            badge.className = 'lesson-completed';
            badge.textContent = '\u2713 Done';
            footer.prepend(badge);
          }
        }
      });
    } catch (e) {}
  }

  /* ── Mini Playground (hero section) ─────────────────── */
  function initMiniPlayground() {
    var runBtn    = document.querySelector('[data-mini-run]');
    var codeArea  = document.querySelector('[data-mini-code]');
    var outputArea= document.querySelector('[data-mini-output]');
    if (!runBtn || !codeArea || !outputArea) return;

    function run() {
      if (typeof runIlma !== 'function') {
        outputArea.innerHTML = '<div class="err-line">Interpreter not loaded.</div>';
        return;
      }
      var code = codeArea.value !== undefined ? codeArea.value : codeArea.textContent;
      var result = runIlma(code);
      outputArea.innerHTML = '';
      if (result.output) {
        result.output.split('\n').forEach(function (line) {
          var span = document.createElement('div');
          span.className = 'out-line';
          span.textContent = line;
          outputArea.appendChild(span);
        });
      }
      if (result.error) {
        var err = document.createElement('div');
        err.className = 'err-line';
        err.textContent = 'Error: ' + result.error;
        outputArea.appendChild(err);
      }
      if (!result.output && !result.error) {
        outputArea.innerHTML = '<div class="hint">(no output)</div>';
      }
    }

    runBtn.addEventListener('click', run);

    if (codeArea.tagName === 'TEXTAREA') {
      codeArea.addEventListener('keydown', function (e) {
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
          e.preventDefault(); run();
        }
        if (e.key === 'Tab') {
          e.preventDefault();
          var start = codeArea.selectionStart, end = codeArea.selectionEnd;
          codeArea.value = codeArea.value.substring(0, start) + '    ' + codeArea.value.substring(end);
          codeArea.selectionStart = codeArea.selectionEnd = start + 4;
        }
      });
    }
  }

  /* ── Init ────────────────────────────────────────────── */
  function init() {
    initStickyNav();
    initActiveNavLink();
    initMobileMenu();
    initCopyButtons();
    initTabs();
    initShowcaseTabs();
    initOSTabs();
    initScrollAnimations();
    initSyntaxHighlighting();
    initAccordion();
    initDocsSidebarTracking();
    initDocsMobileToggle();
    initShareUrl();
    initSearch();
    initPackageRegistry();
    initLearnProgress();
    initMiniPlayground();
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

}());
