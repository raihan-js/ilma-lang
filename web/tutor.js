// ILMA Socratic Tutor — Rule-Based Hint Engine
// Never gives the answer directly. Always asks guiding questions.

class IlmaTutor {
    constructor() {
        this.history = [];
    }

    // Analyze code and provide a hint
    analyze(code, error) {
        if (error) return this.analyzeError(error, code);
        return this.analyzeCode(code);
    }

    analyzeError(error, code) {
        const msg = error.toLowerCase();

        if (msg.includes('expected indent')) {
            return this.hint(
                "It looks like something needs to be indented.",
                "In ILMA, after a colon (:), the next line needs to be indented with spaces. Can you check if the lines inside your block are indented?"
            );
        }

        if (msg.includes('expected colon') || msg.includes('expected :')) {
            return this.hint(
                "Hmm, I think a colon is missing.",
                "In ILMA, blocks like `if`, `repeat`, `recipe`, and `for each` need a colon (:) at the end of the line. Can you find the line that's missing one?"
            );
        }

        if (msg.includes("don't know what") || msg.includes('not defined')) {
            const match = msg.match(/"([^"]+)"/);
            const name = match ? match[1] : 'that';
            return this.hint(
                `I see you're using "${name}" but ILMA doesn't know what it is yet.`,
                `Before using a name, you need to introduce it with \`remember\`. Have you done \`remember ${name} = ...\` before this line?`
            );
        }

        if (msg.includes("don't know the recipe") || msg.includes('recipe')) {
            return this.hint(
                "It seems like a recipe (function) hasn't been defined yet.",
                "In ILMA, you need to define a recipe before you can use it. Have you written `recipe name(...):` somewhere above this line?"
            );
        }

        if (msg.includes('divide by zero')) {
            return this.hint(
                "Oops — dividing by zero!",
                "You can't split something into zero groups. Can you check what value the divisor has? Maybe add an `if` check before dividing: `if b is not 0:`"
            );
        }

        if (msg.includes('unexpected')) {
            return this.hint(
                "Something unexpected appeared in your code.",
                "This usually means there's a typo or something is in the wrong order. Can you read the line carefully and check if every word and symbol is correct?"
            );
        }

        if (msg.includes('took too long') || msg.includes('infinite loop')) {
            return this.hint(
                "Your program seems to be running forever!",
                "This usually means a loop never stops. Check your `keep going while` or `repeat` — does the condition ever become false? Is the counter changing inside the loop?"
            );
        }

        // Generic error hint
        return this.hint(
            "Something went wrong, but that's okay!",
            "Read the error message carefully — it tells you the line number. Go to that line and check: Are all words spelled correctly? Are brackets and quotes matched? Does the indentation look right?"
        );
    }

    analyzeCode(code) {
        const lines = code.split('\n').filter(l => l.trim() && !l.trim().startsWith('#'));

        // Empty program
        if (lines.length === 0) {
            return this.hint(
                "Your program is empty!",
                "Try starting with `say \"Bismillah\"` — it's the simplest ILMA program. What would you like your program to do?"
            );
        }

        // Very short program — encourage exploration
        if (lines.length <= 2) {
            return this.hint(
                "Good start!",
                "You've written a short program. What else could you add? Could you use `remember` to store a value, or `repeat` to do something multiple times?"
            );
        }

        // Detect common patterns and give contextual hints
        if (code.includes('remember') && !code.includes('say')) {
            return this.hint(
                "I see you're remembering values but not showing them.",
                "You've used `remember` to store data — great! But how will you see the result? Try adding `say` to display your values."
            );
        }

        if (code.includes('recipe') && !code.includes('give back')) {
            return this.hint(
                "Your recipe doesn't give anything back.",
                "If your recipe calculates something, you can use `give back` to return the result. Does your recipe need to return a value?"
            );
        }

        if (code.includes('bag[') && !code.includes('for each')) {
            return this.hint(
                "You have a bag but aren't going through it.",
                "Bags are great with `for each` loops! Could you use `for each item in your_bag:` to process each item?"
            );
        }

        if (code.includes('if') && !code.includes('otherwise')) {
            return this.hint(
                "Your `if` doesn't have an `otherwise`.",
                "What should happen when the condition is NOT true? You might want to add `otherwise:` to handle that case."
            );
        }

        if (code.includes('use finance') && !code.includes('zakat')) {
            return this.hint(
                "You're using the finance module!",
                "Have you tried `finance.zakat()` to calculate zakat? Or `finance.compound()` to see the power of compound interest?"
            );
        }

        // Encouragement for good code
        if (code.includes('recipe') && code.includes('give back') && code.includes('for each')) {
            return this.hint(
                "MashaAllah, your code is getting sophisticated!",
                "You're using recipes, return values, and loops — that's real programming. Could you add error handling with `try`/`when wrong` to make it more robust?"
            );
        }

        // Default — general encouragement
        const tips = [
            { title: "Keep going!", message: "Your code looks good. Could you add a comment (starting with #) to explain what each part does? Good programmers always document their code." },
            { title: "Nice work!", message: "Think about edge cases — what happens if a number is zero? What if a bag is empty? Adding checks makes your program more robust." },
            { title: "Looking good!", message: "Have you tried using a knowledge module? `use finance`, `use science`, or `use quran` open up whole new possibilities." },
            { title: "Well done!", message: "Could you turn part of your code into a recipe? Recipes make code reusable — write it once, use it many times." },
        ];
        return tips[Math.floor(Math.random() * tips.length)];
    }

    hint(title, message) {
        return { title, message };
    }
}
