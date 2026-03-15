/* ═══════════════════════════════════════════════════════
   ILMA Website — main.js
   Mobile nav, tabs, OS detection, copy buttons, smooth
   scroll, playground runner, docs sidebar active state.
   ═══════════════════════════════════════════════════════ */

document.addEventListener('DOMContentLoaded', function () {

    /* ── Mobile Navigation ── */
    var hamburger = document.querySelector('.hamburger');
    var navLinks = document.querySelector('.nav-links');

    if (hamburger && navLinks) {
        hamburger.addEventListener('click', function () {
            hamburger.classList.toggle('active');
            navLinks.classList.toggle('open');
        });

        navLinks.querySelectorAll('a').forEach(function (link) {
            link.addEventListener('click', function () {
                hamburger.classList.remove('active');
                navLinks.classList.remove('open');
            });
        });
    }

    /* ── Generic Tab Switching ── */
    function setupTabs(tabSelector, panelSelector) {
        var tabs = document.querySelectorAll(tabSelector);
        var panels = document.querySelectorAll(panelSelector);
        if (!tabs.length) return;

        tabs.forEach(function (tab) {
            tab.addEventListener('click', function () {
                var target = tab.getAttribute('data-tab');
                tabs.forEach(function (t) { t.classList.remove('active'); });
                panels.forEach(function (p) { p.classList.remove('active'); });
                tab.classList.add('active');
                var panel = document.getElementById(target);
                if (panel) panel.classList.add('active');
            });
        });
    }

    setupTabs('.tier-tab', '.tier-panel');
    setupTabs('.install-tab', '.install-panel');

    /* ── OS Detection for Install Page ── */
    var installTabs = document.querySelectorAll('.install-tab');
    if (installTabs.length) {
        var ua = navigator.userAgent.toLowerCase();
        var osTab = 'tab-linux';
        if (ua.indexOf('mac') !== -1) osTab = 'tab-macos';
        else if (ua.indexOf('win') !== -1) osTab = 'tab-windows';

        installTabs.forEach(function (t) { t.classList.remove('active'); });
        document.querySelectorAll('.install-panel').forEach(function (p) { p.classList.remove('active'); });

        var autoTab = document.querySelector('.install-tab[data-tab="' + osTab + '"]');
        var autoPanel = document.getElementById(osTab);
        if (autoTab) autoTab.classList.add('active');
        if (autoPanel) autoPanel.classList.add('active');
    }

    /* ── Copy Buttons ── */
    document.querySelectorAll('.copy-btn').forEach(function (btn) {
        btn.addEventListener('click', function () {
            var block = btn.closest('.install-code-block') || btn.closest('.docs-code');
            if (!block) return;
            var pre = block.querySelector('pre');
            if (!pre) return;
            var text = pre.textContent;
            navigator.clipboard.writeText(text).then(function () {
                btn.textContent = 'Copied!';
                btn.classList.add('copied');
                setTimeout(function () {
                    btn.textContent = 'Copy';
                    btn.classList.remove('copied');
                }, 2000);
            }).catch(function () {
                /* Fallback for older browsers */
                var ta = document.createElement('textarea');
                ta.value = text;
                ta.style.position = 'fixed';
                ta.style.left = '-9999px';
                document.body.appendChild(ta);
                ta.select();
                document.execCommand('copy');
                document.body.removeChild(ta);
                btn.textContent = 'Copied!';
                btn.classList.add('copied');
                setTimeout(function () {
                    btn.textContent = 'Copy';
                    btn.classList.remove('copied');
                }, 2000);
            });
        });
    });

    /* ── Smooth Scroll for Anchor Links ── */
    document.querySelectorAll('a[href^="#"]').forEach(function (link) {
        link.addEventListener('click', function (e) {
            var href = link.getAttribute('href');
            if (href === '#') return;
            var target = document.querySelector(href);
            if (target) {
                e.preventDefault();
                target.scrollIntoView({ behavior: 'smooth' });
            }
        });
    });

    /* ── Playground ── */
    var runBtn = document.getElementById('run-btn');
    var editorEl = document.getElementById('ilma-editor');
    var outputEl = document.getElementById('ilma-output');

    if (runBtn && editorEl && outputEl) {
        runBtn.addEventListener('click', function () {
            outputEl.className = 'playground-output';
            outputEl.textContent = '';
            var code = editorEl.value;
            if (!code.trim()) {
                outputEl.textContent = '(no code to run)';
                return;
            }

            if (typeof runIlma === 'function') {
                var result = runIlma(code);
                if (result.error) {
                    outputEl.innerHTML = '<span class="error">' + escapeHtml(result.error) + '</span>';
                } else {
                    outputEl.textContent = result.output || '(no output)';
                }
            } else {
                outputEl.innerHTML = '<span class="error">Interpreter not loaded.</span>';
            }
        });

        /* Tab key support in editor */
        editorEl.addEventListener('keydown', function (e) {
            if (e.key === 'Tab') {
                e.preventDefault();
                var start = editorEl.selectionStart;
                var end = editorEl.selectionEnd;
                editorEl.value = editorEl.value.substring(0, start) + '    ' + editorEl.value.substring(end);
                editorEl.selectionStart = editorEl.selectionEnd = start + 4;
            }
        });
    }

    function escapeHtml(str) {
        var div = document.createElement('div');
        div.appendChild(document.createTextNode(str));
        return div.innerHTML;
    }

    /* ── Docs Sidebar Active State on Scroll ── */
    var docsSections = document.querySelectorAll('.docs-section');
    var sidebarLinks = document.querySelectorAll('.docs-sidebar a');

    if (docsSections.length && sidebarLinks.length) {
        function updateActiveSection() {
            var scrollPos = window.scrollY + 100;
            docsSections.forEach(function (section) {
                var top = section.offsetTop;
                var bottom = top + section.offsetHeight;
                var id = section.getAttribute('id');
                if (scrollPos >= top && scrollPos < bottom) {
                    sidebarLinks.forEach(function (link) {
                        link.classList.remove('active');
                        if (link.getAttribute('href') === '#' + id) {
                            link.classList.add('active');
                        }
                    });
                }
            });
        }

        window.addEventListener('scroll', updateActiveSection);
        updateActiveSection();
    }

    /* ── Docs Sidebar Toggle (Mobile) ── */
    var sidebarToggle = document.querySelector('.docs-sidebar-toggle');
    var docsSidebar = document.querySelector('.docs-sidebar');

    if (sidebarToggle && docsSidebar) {
        sidebarToggle.addEventListener('click', function () {
            docsSidebar.classList.toggle('open');
            sidebarToggle.textContent = docsSidebar.classList.contains('open') ? '\u2715' : '\u2630';
        });

        docsSidebar.querySelectorAll('a').forEach(function (link) {
            link.addEventListener('click', function () {
                if (window.innerWidth <= 768) {
                    docsSidebar.classList.remove('open');
                    sidebarToggle.textContent = '\u2630';
                }
            });
        });
    }
});
