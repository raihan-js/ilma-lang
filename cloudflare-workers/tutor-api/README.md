# ILMA Tutor Worker

A zero-cost AI tutor powered by Cloudflare Workers AI.

## Cost
FREE on Cloudflare's Workers free plan:
- 100,000 Worker requests/day
- 10,000 AI neurons/day (~200 student conversations)

Upgrade to Workers Paid ($5/month) for unlimited requests if needed.

## Deploy (one-time setup, ~5 minutes)

### Prerequisites
- Cloudflare account (you already have one for Pages)
- Node.js installed

### Steps
```bash
cd cloudflare-workers/tutor-api
npm install

# Log in to Cloudflare
npx wrangler login

# Deploy the Worker
npx wrangler deploy
```

After deploy, Wrangler prints your Worker URL:
`https://ilma-tutor.YOUR-SUBDOMAIN.workers.dev`

### Connect to website
Copy that URL and update `website/js/tutor-config.js`:
```js
const ILMA_TUTOR_CONFIG = {
  endpoint: "https://ilma-tutor.YOUR-SUBDOMAIN.workers.dev",
  enabled: true,
  ...
};
```

### Allow your domain
In `src/index.js`, the CORS header is set to `"https://ilma-lang.dev"`.
Change this if your domain is different.

### Test it
```bash
curl -X POST https://ilma-tutor.YOUR-SUBDOMAIN.workers.dev \
  -H "Content-Type: application/json" \
  -d '{"message": "How do I print something in ILMA?"}'
```

## Development (local testing)
```bash
npx wrangler dev
# Worker runs at http://localhost:8787
```

## Monitor usage
Cloudflare Dashboard → Workers & Pages → ilma-tutor → Metrics

Free tier resets daily at midnight UTC.
