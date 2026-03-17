/* ILMA Tutor Configuration
   Replace TUTOR_ENDPOINT with your deployed Worker URL after running:
   cd cloudflare-workers/tutor-api && npx wrangler deploy
*/
const ILMA_TUTOR_CONFIG = {
  endpoint: "https://ilma-tutor.araihansikder.workers.dev",
  enabled: true,         // Deployed 2026-03-18
  maxHistoryMessages: 4, // Keep last 4 messages for context
  timeoutMs: 8000        // 8 second timeout before showing fallback
};

/* Rate limiting (client-side, soft limit) */
const TUTOR_RATE_LIMIT = {
  maxRequestsPerHour: 20,
  storageKey: "ilma_tutor_requests"
};
