// ILMA AI Tutor — Connects to Cloudflare Workers AI with rule-based fallback

class IlmaAITutor {
  constructor() {
    this.endpoint = null;
    this.enabled  = false;
    this.history  = [];
    this.fallback = new IlmaTutor(); // existing rule-based tutor
    this._requestsThisHour = 0;
    this._hourStart = Date.now();
  }

  configure(config) {
    this.endpoint = config.endpoint;
    this.enabled  = config.enabled === true && !!config.endpoint
                    && !config.endpoint.includes("YOUR-SUBDOMAIN");
  }

  _checkRateLimit() {
    const now = Date.now();
    if (now - this._hourStart > 3600000) {
      this._requestsThisHour = 0;
      this._hourStart = now;
    }
    if (this._requestsThisHour >= 20) return false;
    this._requestsThisHour++;
    return true;
  }

  async ask(userMessage, code, lessonTitle) {
    if (!this._checkRateLimit()) {
      return {
        message: "You're asking great questions! Take a moment to try what we discussed, then come back.",
        source: "rate-limited"
      };
    }

    // Try AI if configured
    if (this.enabled && this.endpoint) {
      try {
        const response = await fetch(this.endpoint, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            message: userMessage,
            code: code || "",
            lesson: lessonTitle || "",
            history: this.history.slice(-4)
          }),
          signal: AbortSignal.timeout(8000)
        });

        if (response.ok) {
          const data = await response.json();
          if (data.reply) {
            this.history.push(
              { role: "user", content: userMessage },
              { role: "assistant", content: data.reply }
            );
            if (this.history.length > 10) this.history = this.history.slice(-10);
            return { message: data.reply, source: "ai" };
          }
        }
      } catch (err) {
        // Network error or timeout — fall through to rule-based
      }
    }

    // Rule-based fallback
    const hint = this.fallback.analyze(code || "", null);
    return { message: hint.message, source: "fallback" };
  }

  resetConversation() {
    this.history = [];
  }
}
