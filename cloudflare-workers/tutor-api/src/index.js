const SYSTEM_PROMPT = `You are the ILMA tutor — a kind, patient
teacher helping children learn to code in ILMA.

ILMA is a programming language where:
- say = print to screen
- remember = create a variable
- recipe = define a function
- give back = return a value
- blueprint = define a class
- me = self/this inside a blueprint
- comes from = inheritance
- bag = a list of items
- notebook = a key-value store
- for each = loop over items
- keep going while = while loop
- repeat N: = repeat N times
- shout = raise an error
- try / when wrong = try/catch
- use finance = import the finance module
- use time, use body, use science, use quran, use draw, use number, use think, use trade

YOUR STRICT RULES — never break these:
1. NEVER give the full answer or write the complete corrected code
2. Ask ONE guiding question per response
3. Start by finding something right in their code first
4. Use simple words a 10-year-old understands
5. Keep responses under 80 words
6. Always end with a question
7. Be warm and encouraging — learning is hard and that is okay
8. If they are totally stuck, give a tiny hint (one keyword) not the solution

Good response example:
Student: "how do I print something?"
Tutor: "Great question! In ILMA we use the word 'say' to show things
on screen — just like saying something out loud. What do you want to show?"`;

export default {
  async fetch(request, env) {
    // CORS headers — allow requests from ilma-lang.dev
    const corsHeaders = {
      "Access-Control-Allow-Origin": "https://ilma-lang.dev",
      "Access-Control-Allow-Methods": "POST, OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type",
      "Content-Type": "application/json"
    };

    // Handle preflight
    if (request.method === "OPTIONS") {
      return new Response(null, { status: 204, headers: corsHeaders });
    }

    if (request.method !== "POST") {
      return new Response(
        JSON.stringify({ error: "POST only" }),
        { status: 405, headers: corsHeaders }
      );
    }

    try {
      const body = await request.json();
      const userMessage = (body.message || "").slice(0, 500);
      const code        = (body.code    || "").slice(0, 1000);
      const lesson      = (body.lesson  || "").slice(0, 100);
      const history     = Array.isArray(body.history)
                          ? body.history.slice(-4)
                          : [];

      if (!userMessage && !code) {
        return new Response(
          JSON.stringify({ error: "No message provided" }),
          { status: 400, headers: corsHeaders }
        );
      }

      // Build the user turn
      let userTurn = userMessage;
      if (code) {
        userTurn += `\n\nMy current code:\n\`\`\`\n${code}\n\`\`\``;
      }
      if (lesson) {
        userTurn += `\n(I am working on: ${lesson})`;
      }

      // Build messages array for the model
      const messages = [
        { role: "system", content: SYSTEM_PROMPT },
        ...history,
        { role: "user", content: userTurn }
      ];

      // Call Cloudflare Workers AI — Llama 3.1 8B (fast, free tier)
      const aiResponse = await env.AI.run(
        "@cf/meta/llama-3.1-8b-instruct-fast",
        {
          messages,
          max_tokens: 150,
          temperature: 0.7
        }
      );

      const reply = (aiResponse.response || "")
        .trim()
        .replace(/^(Assistant:|ILMA Tutor:)\s*/i, "");

      return new Response(
        JSON.stringify({ reply, ok: true }),
        { headers: corsHeaders }
      );

    } catch (err) {
      // Friendly fallback — don't expose internal errors
      return new Response(
        JSON.stringify({
          reply: "I'm thinking... try asking me in a different way!",
          ok: true,
          fallback: true
        }),
        { headers: corsHeaders }
      );
    }
  }
};
