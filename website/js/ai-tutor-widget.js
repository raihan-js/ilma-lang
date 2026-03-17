(function() {
  'use strict';
  const tutor = {
    endpoint: null,
    enabled: false,
    history: [],
    requests: 0,
    hourStart: Date.now(),

    init: function() {
      if (typeof ILMA_TUTOR_CONFIG !== 'undefined') {
        this.endpoint = ILMA_TUTOR_CONFIG.endpoint;
        this.enabled  = ILMA_TUTOR_CONFIG.enabled === true
                        && this.endpoint
                        && !this.endpoint.includes("YOUR-SUBDOMAIN");
      }
      var btn   = document.getElementById('playground-tutor-send');
      var input = document.getElementById('playground-tutor-input');
      if (!btn || !input) return;
      btn.addEventListener('click', () => this.send());
      input.addEventListener('keydown', (e) => { if (e.key === 'Enter') this.send(); });
    },

    getCode: function() {
      if (window.ilmaEditor) return window.ilmaEditor.getValue() || '';
      var ta = document.getElementById('playground-editor');
      return ta ? ta.value : '';
    },

    addMessage: function(text, isUser) {
      var msgs = document.getElementById('playground-tutor-messages');
      if (!msgs) return;
      var div = document.createElement('div');
      div.style.cssText = 'margin-bottom:12px;padding:8px 10px;border-radius:8px;'
        + (isUser ? 'background:rgba(83,74,183,0.2);text-align:right;font-style:italic'
                  : 'background:rgba(255,255,255,0.05)');
      div.textContent = text;
      msgs.appendChild(div);
      msgs.scrollTop = msgs.scrollHeight;
    },

    send: async function() {
      var input = document.getElementById('playground-tutor-input');
      var msg   = (input.value || '').trim();
      if (!msg) return;
      input.value = '';

      this.addMessage(msg, true);

      if (!this.enabled) {
        this.addMessage(
          "The AI tutor is not set up yet. In the meantime: " +
          "try reading the error message carefully — it usually tells you exactly what went wrong!",
          false
        );
        return;
      }

      // Rate limit
      if (Date.now() - this.hourStart > 3600000) { this.requests = 0; this.hourStart = Date.now(); }
      if (this.requests >= 20) {
        this.addMessage("You've asked lots of great questions! Take a short break and try what we discussed.", false);
        return;
      }
      this.requests++;

      var msgs = document.getElementById('playground-tutor-messages');
      var thinking = document.createElement('div');
      thinking.style.cssText = 'margin-bottom:12px;padding:8px 10px;border-radius:8px;background:rgba(255,255,255,0.05);opacity:0.5;font-style:italic';
      thinking.textContent = 'Thinking...';
      msgs.appendChild(thinking);

      try {
        var res = await fetch(this.endpoint, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            message: msg,
            code: this.getCode(),
            history: this.history.slice(-4)
          }),
          signal: AbortSignal.timeout(8000)
        });
        thinking.remove();
        if (res.ok) {
          var data = await res.json();
          var reply = data.reply || 'Try experimenting with your code!';
          this.history.push({role:'user',content:msg},{role:'assistant',content:reply});
          if (this.history.length > 10) this.history = this.history.slice(-10);
          this.addMessage(reply, false);
        } else {
          this.addMessage("Having trouble connecting. Try reading the docs — they might have the answer!", false);
        }
      } catch(e) {
        thinking.remove();
        this.addMessage("Connection timed out. Try asking in a different way!", false);
      }
    }
  };

  document.addEventListener('DOMContentLoaded', function() { tutor.init(); });
})();
