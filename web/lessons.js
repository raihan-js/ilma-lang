// ILMA Lesson Data — First 10 Lessons (Tier 1 & Early Tier 2)
// Based on the ILMA Blueprint v2 curriculum

const LESSONS = [
    {
        id: 1,
        title: "Hello, World!",
        subtitle: "Your very first program",
        concept: "say",
        body: `
<p>Welcome to ILMA! In ILMA, when you want the computer to show something on screen, you use the word <code>say</code>.</p>

<p>Every great journey begins with <strong>Bismillah</strong>. Let's write your very first program:</p>

<div class="hint">Try clicking the <strong>Run</strong> button to see what happens!</div>

<p>The <code>say</code> command tells the computer: "Show this text to me." Whatever you put in quotes after <code>say</code> will appear in the output below.</p>

<p><strong>Try it:</strong> Change the text inside the quotes to say your own name!</p>
`,
        code: `# My very first ILMA program
say "Bismillah"
say "Hello, World!"
say "My name is ILMA"
`,
        expected: "Bismillah\nHello, World!\nMy name is ILMA"
    },
    {
        id: 2,
        title: "Remembering Things",
        subtitle: "Variables store information",
        concept: "remember",
        body: `
<p>Computers can <strong>remember</strong> things for you. In ILMA, you use <code>remember</code> to store a value with a name.</p>

<p>Think of it like writing something on a sticky note and giving it a label.</p>

<div class="hint"><code>remember name = "Yusuf"</code> means: "Remember that the name is Yusuf."</div>

<p>You can remember numbers, text, and more. Then you can use that name later in your program.</p>

<p><strong>Try it:</strong> Change the values and add your own <code>remember</code> lines!</p>
`,
        code: `# Remembering things
remember my_name = "Amira"
remember my_age = 10
remember my_city = "London"

say "My name is " + my_name
say "I am " + my_age + " years old"
say "I live in " + my_city
`,
        expected: 'My name is Amira\nI am 10 years old\nI live in London'
    },
    {
        id: 3,
        title: "Numbers and Maths",
        subtitle: "Adding, subtracting, and more",
        concept: "arithmetic",
        body: `
<p>ILMA can do maths! You can use <code>+</code>, <code>-</code>, <code>*</code> (multiply), and <code>/</code> (divide).</p>

<p>When you combine numbers, ILMA calculates the answer for you.</p>

<div class="hint">Maths is the language of the universe. Every calculation you do here is the same one a bank or a spaceship uses.</div>

<p><strong>Try it:</strong> Can you calculate how much money you'd have if you saved 5 coins every day for a week?</p>
`,
        code: `# Numbers and maths
remember apples = 10
remember eaten = 3
remember left = apples - eaten
say "Apples left: " + left

# More maths
say "5 + 3 = " + (5 + 3)
say "10 * 4 = " + (10 * 4)
say "100 / 5 = " + (100 / 5)

# Savings calculator
remember daily_saving = 5
remember days = 7
say "After a week: " + (daily_saving * days) + " coins"
`,
        expected: 'Apples left: 7\n5 + 3 = 8\n10 * 4 = 40\n100 / 5 = 20\nAfter a week: 35 coins'
    },
    {
        id: 4,
        title: "Making Decisions",
        subtitle: "if and otherwise",
        concept: "if/otherwise",
        body: `
<p>Programs can make <strong>decisions</strong>. In ILMA, you use <code>if</code> to check something, and <code>otherwise</code> for what happens when it's not true.</p>

<p>Think about it: "If it's raining, take an umbrella. Otherwise, wear sunglasses."</p>

<div class="hint">Every decision has a <strong>condition</strong> (the check) and a <strong>consequence</strong> (what happens). This is how all software works!</div>

<p><strong>Try it:</strong> Change the score to different numbers and see what message appears!</p>
`,
        code: `# Making decisions
remember score = 85

if score >= 90:
    say "Excellent! MashaAllah!"
otherwise if score >= 70:
    say "Good effort, keep going!"
otherwise:
    say "Don't give up, practise more!"

# Another decision
remember temperature = 35
if temperature > 30:
    say "It's hot outside! Drink water."
otherwise:
    say "The weather is nice."
`,
        expected: 'Good effort, keep going!\nIt\'s hot outside! Drink water.'
    },
    {
        id: 5,
        title: "Asking Questions",
        subtitle: "Getting input from the user",
        concept: "ask",
        body: `
<p>Programs can <strong>ask</strong> questions! In ILMA, <code>ask</code> shows a question and waits for an answer.</p>

<p>The answer is stored in a variable, just like <code>remember</code>.</p>

<div class="hint">Good questions are a sign of intelligence. The Prophet (peace be upon him) said: "The cure for ignorance is to ask."</div>

<p><strong>Try it:</strong> Run the program and type your name when asked!</p>
`,
        code: `# Asking questions
remember name = ask "What is your name? "
say "Assalamu Alaikum, " + name + "!"
say "Welcome to ILMA."

remember age = ask "How old are you? "
say name + " is " + age + " years old."
`,
        expected: null // Interactive — no fixed expected output
    },
    {
        id: 6,
        title: "Doing Things Again",
        subtitle: "Loops with repeat",
        concept: "repeat",
        body: `
<p>Sometimes you want to do something many times. In ILMA, <code>repeat</code> does exactly that.</p>

<p><code>repeat 5:</code> means "do the following 5 times."</p>

<div class="hint">Excellence comes from repetition. The ten-thousand-hour principle: anything you practise enough, you master.</div>

<p><strong>Try it:</strong> Change the number after <code>repeat</code> and see what happens!</p>
`,
        code: `# Doing things again and again
repeat 5:
    say "SubhanAllah"

say "---"

# Counting with repeat
remember count = 0
repeat 10:
    count = count + 1
say "I counted to " + count
`,
        expected: 'SubhanAllah\nSubhanAllah\nSubhanAllah\nSubhanAllah\nSubhanAllah\n---\nI counted to 10'
    },
    {
        id: 7,
        title: "Waiting for the Right Moment",
        subtitle: "keep going while",
        concept: "keep going while",
        body: `
<p>Sometimes you don't know exactly how many times to repeat. You just want to <strong>keep going while</strong> something is true.</p>

<p>Think: "Keep walking while you haven't reached the school."</p>

<div class="hint">Patience and perseverance — <strong>sabr</strong> — is a core value. Keep going while the goal is not yet reached.</div>

<p><strong>Try it:</strong> Can you make the savings reach exactly 100?</p>
`,
        code: `# Keep going while
remember savings = 0

keep going while savings < 100:
    savings = savings + 15

say "Final savings: " + savings

# Countdown
remember rocket = 5
keep going while rocket > 0:
    say rocket + "..."
    rocket = rocket - 1
say "Liftoff!"
`,
        expected: 'Final savings: 105\n5...\n4...\n3...\n2...\n1...\nLiftoff!'
    },
    {
        id: 8,
        title: "Bags of Things",
        subtitle: "Collections with bag",
        concept: "bag",
        body: `
<p>A <code>bag</code> holds a collection of things in order, like a bag of fruits or a list of names.</p>

<p>You can add things, check how many there are, and go through each one.</p>

<div class="hint">Organisation matters. How you manage your resources — even a simple list — is a life skill.</div>

<p><strong>Try it:</strong> Add more fruits to the bag and see them listed!</p>
`,
        code: `# Bags of things
remember fruits = bag["dates", "mango", "apple"]
say "I have " + fruits.size + " fruits"

# Add more
fruits.add("pomegranate")
say "Now I have " + fruits.size + " fruits"

# Go through each one
for each fruit in fruits:
    say "  - " + fruit

# Sorted!
remember sorted = fruits.sorted()
say "Sorted:"
for each fruit in sorted:
    say "  - " + fruit
`,
        expected: 'I have 3 fruits\nNow I have 4 fruits\n  - dates\n  - mango\n  - apple\n  - pomegranate\nSorted:\n  - apple\n  - dates\n  - mango\n  - pomegranate'
    },
    {
        id: 9,
        title: "Going Through Everything",
        subtitle: "for each loops",
        concept: "for each",
        body: `
<p><code>for each</code> lets you go through every item in a bag, one by one.</p>

<p>Think of it as: "For each student in the class, check their homework."</p>

<div class="hint">Justice means treating every item equally. No favourites. <code>for each</code> processes every element the same way.</div>

<p><strong>Try it:</strong> Add more names and see the greetings!</p>
`,
        code: `# Greeting everyone
remember names = bag["Yusuf", "Amira", "Ibrahim", "Fatima"]

for each name in names:
    say "Assalamu Alaikum, " + name + "!"

say "---"

# Summing numbers
remember numbers = bag[10, 20, 30, 40, 50]
remember total = 0
for each num in numbers:
    total = total + num
say "Total: " + total
`,
        expected: 'Assalamu Alaikum, Yusuf!\nAssalamu Alaikum, Amira!\nAssalamu Alaikum, Ibrahim!\nAssalamu Alaikum, Fatima!\n---\nTotal: 150'
    },
    {
        id: 10,
        title: "Recipes",
        subtitle: "Reusable skills with recipe",
        concept: "recipe",
        body: `
<p>A <code>recipe</code> is a reusable skill. You give it a name, teach it how to do something, and then you can use it whenever you want.</p>

<p>Think of a recipe in a cookbook: you write it once and use it many times.</p>

<div class="hint">Skills are reusable. Teaching what you know multiplies its value. A recipe is knowledge you can share.</div>

<p><strong>Try it:</strong> Create your own recipe that calculates something useful!</p>
`,
        code: `# Recipes — reusable skills
recipe greet(name):
    say "Assalamu Alaikum, " + name + "!"

greet("Yusuf")
greet("Amira")

# A recipe that gives back a result
recipe double(n):
    give back n * 2

say "Double 5 is " + double(5)
say "Double 21 is " + double(21)

# A recipe for zakat
recipe calculate_zakat(wealth):
    give back wealth * 0.025

remember my_savings = 10000
remember my_zakat = calculate_zakat(my_savings)
say "Zakat on " + my_savings + " is " + my_zakat
`,
        expected: 'Assalamu Alaikum, Yusuf!\nAssalamu Alaikum, Amira!\nDouble 5 is 10\nDouble 21 is 42\nZakat on 10000 is 250'
    },
    // ═══════════ TIER 2 CURRICULUM (Lessons 11-20) ═══════════
    {
        id: 11,
        title: "The Budget",
        subtitle: "Finance module basics",
        concept: "use finance",
        body: `
<p>ILMA has <strong>knowledge modules</strong> — built-in tools that teach you about the real world while you code.</p>

<p>The <code>finance</code> module helps you learn about money: budgets, profit, and saving.</p>

<div class="hint">The 50/30/20 rule: 50% of income for needs, 30% for savings, 20% for wants. This simple rule can change your financial life.</div>

<p>Use <code>use finance</code> to load the module, then call its functions with <code>finance.function_name()</code>.</p>
`,
        code: `# The Budget — using the finance module
use finance

remember income = 3000
remember budget = finance.budget(income)

say "Monthly income: " + income
say "Needs (50%):    " + budget[needs]
say "Savings (30%):  " + budget[savings]
say "Wants (20%):    " + budget[wants]

# Profit calculation
remember profit = finance.profit(200, 340)
say "Profit: " + profit

# Profit margin
remember margin = finance.margin(200, 340)
say "Margin: " + margin + "%"
`,
        expected: 'Monthly income: 3000\nNeeds (50%):    1500\nSavings (30%):  900\nWants (20%):    600\nProfit: 140\nMargin: 41.18%'
    },
    {
        id: 12,
        title: "Zakat",
        subtitle: "The mathematics of giving",
        concept: "finance.zakat",
        body: `
<p><strong>Zakat</strong> is one of the five pillars of Islam. It means giving 2.5% of your wealth to those in need — but only if your savings exceed the <strong>nisab</strong> (minimum threshold).</p>

<p>The <code>finance.zakat()</code> function calculates this for you.</p>

<div class="hint">Zakat purifies your wealth. It is not charity — it is an obligation, a right that the poor have over the rich. The maths: wealth &times; 0.025.</div>

<p><strong>Try it:</strong> Change the savings amount and see when zakat becomes due!</p>
`,
        code: `# Zakat Calculator
use finance

remember savings = 5000
remember nisab = 595

remember zakat = finance.zakat(savings, nisab)

if zakat > 0:
    say "Your savings: " + savings
    say "Zakat due: " + zakat
    say "Paying zakat purifies your wealth."
otherwise:
    say "Your savings: " + savings
    say "Below nisab. No zakat due yet."

# What about a smaller amount?
say "---"
remember small = 400
remember small_zakat = finance.zakat(small, nisab)
say "Savings: " + small + ", Zakat: " + small_zakat
`,
        expected: 'Your savings: 5000\nZakat due: 125\nPaying zakat purifies your wealth.\n---\nSavings: 400, Zakat: 0'
    },
    {
        id: 13,
        title: "Compound Interest",
        subtitle: "Why starting early changes everything",
        concept: "finance.compound",
        body: `
<p><strong>Compound interest</strong> is the most powerful concept in finance. It means earning interest on your interest.</p>

<p><code>finance.compound(principal, rate, years)</code> shows you what happens when money grows over time.</p>

<div class="hint">Albert Einstein called compound interest "the eighth wonder of the world." Start saving early, even small amounts, and time does the heavy lifting.</div>

<p><strong>Try it:</strong> Compare saving for 10 years vs. 30 years!</p>
`,
        code: `# Compound Interest — the power of time
use finance

remember start = 1000
remember rate = 0.05

say "Starting with: " + start
say "Rate: 5% per year"
say "---"

# After 10 years
remember after_10 = finance.compound(start, rate, 10)
say "After 10 years: " + after_10

# After 20 years
remember after_20 = finance.compound(start, rate, 20)
say "After 20 years: " + after_20

# After 30 years
remember after_30 = finance.compound(start, rate, 30)
say "After 30 years: " + after_30

say "---"
say "The money grew " + (after_30 / start) + "x in 30 years!"
`,
        expected: 'Starting with: 1000\nRate: 5% per year\n---\nAfter 10 years: 1628.89\nAfter 20 years: 2653.3\nAfter 30 years: 4321.94\n---\nThe money grew 4.32194x in 30 years!'
    },
    {
        id: 14,
        title: "Writing and Text",
        subtitle: "Text operations and power of words",
        concept: "text methods",
        body: `
<p>Text (strings) in ILMA have powerful methods. You can change case, search, slice, and join text.</p>

<div class="hint">Words have power. Writing clearly is a life skill. The Quran's very first command was <strong>Iqra</strong> — Read. Learning to manipulate text is learning to communicate.</div>

<p>Methods: <code>.upper()</code>, <code>.lower()</code>, <code>.contains()</code>, <code>.slice()</code>, <code>.length</code>, <code>separator.join(bag)</code></p>
`,
        code: `# Text operations
remember greeting = "Assalamu Alaikum"

# Upper and lower case
say greeting.upper()
say greeting.lower()

# Length
say "Length: " + greeting.length

# Slice (extract part of text)
say greeting.slice(0, 8)

# Contains
if greeting.contains("Alaikum"):
    say "Found 'Alaikum' in the greeting!"

# Join — combine a bag into text
remember words = bag["In", "the", "name", "of", "Allah"]
say " ".join(words)
`,
        expected: 'ASSALAMU ALAIKUM\nassalamu alaikum\nLength: 16\nAssalamu\nFound \'Alaikum\' in the greeting!\nIn the name of Allah'
    },
    {
        id: 15,
        title: "The Islamic Calendar",
        subtitle: "Hijri dates in code",
        concept: "use time",
        body: `
<p>The <code>time</code> module gives you today's date and can convert to the <strong>Hijri (Islamic) calendar</strong>.</p>

<div class="hint">The Islamic calendar is lunar — based on the moon. It started with the Hijra of the Prophet (peace be upon him) from Makkah to Madinah. Every Muslim should know their Hijri date.</div>

<p>Functions: <code>time.today()</code>, <code>time.to_hijri()</code>, <code>time.days_between()</code></p>
`,
        code: `# The Islamic Calendar
use time

remember today = time.today()
say "Today (Gregorian): " + today

remember hijri = time.to_hijri(today)
say "Today (Hijri): " + hijri

# How many days between two dates?
remember days = time.days_between("2025-01-01", "2025-12-31")
say "Days in 2025: " + days

say "Current year: " + time.year()
`,
        expected: null  // Dynamic dates — can't predict output
    },
    {
        id: 16,
        title: "Looking After Your Body",
        subtitle: "Health calculations",
        concept: "use body",
        body: `
<p>The <code>body</code> module helps you understand health metrics like BMI and water intake.</p>

<div class="hint">Your body is an <strong>amanah</strong> — a trust from Allah. Taking care of it is an act of worship. Know your numbers: BMI, water, sleep.</div>

<p>Functions: <code>body.bmi()</code>, <code>body.bmi_category()</code>, <code>body.daily_water()</code></p>
`,
        code: `# Looking after your body
use body

remember weight = 70
remember height = 175

remember my_bmi = body.bmi(weight, height)
remember category = body.bmi_category(my_bmi)

say "Weight: " + weight + " kg"
say "Height: " + height + " cm"
say "BMI: " + my_bmi + " (" + category + ")"

# Daily water goal
remember water = body.daily_water(weight)
say "Daily water goal: " + water + " litres"
`,
        expected: 'Weight: 70 kg\nHeight: 175 cm\nBMI: 22.9 (healthy)\nDaily water goal: 2.3 litres'
    },
    {
        id: 17,
        title: "Notebooks",
        subtitle: "Labelled collections",
        concept: "notebook",
        body: `
<p>A <code>notebook</code> stores labelled information — like a real notebook with headings and notes next to them.</p>

<p>Create with <code>notebook[key: value, ...]</code>, access with <code>book[key]</code>, iterate with <code>for each</code>.</p>

<div class="hint">Key-value thinking is everywhere: a dictionary has words and definitions, a contact list has names and numbers, a database has records and fields.</div>
`,
        code: `# Notebooks — labelled collections
remember student = notebook[name: "Amira", age: 12, grade: "A", city: "London"]

say "Name: " + student[name]
say "Age: " + student[age]
say "Grade: " + student[grade]

# Loop through all entries
say "---"
for each key in student:
    say key + " = " + student[(key)]

# Notebooks in real life
remember meal = notebook[dish: "Biryani", time: 45, servings: 4]
say "---"
say meal[dish] + " takes " + meal[time] + " minutes for " + meal[servings] + " people"
`,
        expected: 'Name: Amira\nAge: 12\nGrade: A\n---\nname = Amira\nage = 12\ngrade = A\ncity = London\n---\nBiryani takes 45 minutes for 4 people'
    },
    {
        id: 18,
        title: "When Things Go Wrong",
        subtitle: "Error handling with try/when wrong",
        concept: "try/shout/when wrong",
        body: `
<p>Mistakes happen — in code and in life. ILMA handles errors with <code>try</code> and <code>when wrong</code>.</p>

<p>Use <code>shout</code> to signal an error. Use <code>try</code>/<code>when wrong</code> to catch it gracefully.</p>

<div class="hint">Mistakes are part of learning. How to handle failure with grace and fix it — that is wisdom. The Prophet (peace be upon him) said: "Every son of Adam makes mistakes, and the best of those who make mistakes are those who repent."</div>
`,
        code: `# When things go wrong
recipe divide(a, b):
    if b is 0:
        shout "Cannot divide by zero!"
    give back a / b

# This works fine
say "10 / 2 = " + divide(10, 2)

# This would crash — but we catch it
try:
    say divide(10, 0)
when wrong:
    say "Oops! Caught an error."

say "Program continues safely."

# Nested try
try:
    try:
        shout "inner error"
    when wrong:
        say "Inner catch"
    say "After inner try"
when wrong:
    say "Outer catch"
`,
        expected: '10 / 2 = 5\nOops! Caught an error.\nProgram continues safely.\nInner catch\nAfter inner try'
    },
    {
        id: 19,
        title: "The Thinking Module",
        subtitle: "Stoic wisdom in code",
        concept: "use think",
        body: `
<p>The <code>think</code> module teaches you to think clearly. It includes Stoic wisdom — the ancient philosophy of self-mastery.</p>

<div class="hint">Stoicism teaches: separate what is <strong>in your control</strong> from what is <strong>not</strong>. Your effort is in your control. The result is not. Focus on the effort.</div>

<p>The <code>think.stoic_question()</code> gives you a daily reflection question.</p>
`,
        code: `# The Thinking Module
use think

# Get a daily stoic question
remember question = think.stoic_question()
say "Today's reflection:"
say question

# Decision making with code
remember choice = "study"
if choice is "study":
    say "---"
    say "You chose to study."
    say "Knowledge compounds, just like interest."
    say "Every hour you invest in learning pays dividends forever."
`,
        expected: null  // Random question — can't predict
    },
    {
        id: 20,
        title: "My First Project",
        subtitle: "Putting it all together",
        concept: "combining everything",
        body: `
<p>You've learned <strong>everything</strong> in Tier 1 and Tier 2. Now it's time to build something real.</p>

<p>This program combines: variables, decisions, loops, bags, recipes, and the finance module. It's a complete savings tracker.</p>

<div class="hint">Identify a problem in your family or community. Solve it with ILMA. That is the purpose of programming — to make the world better.</div>

<p><strong>Challenge:</strong> Modify this program to track YOUR savings goal!</p>
`,
        code: `# My Savings Tracker — combining everything
use finance

remember goal = 1000
remember saved = 0
remember weekly_saving = 50
remember weeks = 0

say "=== Savings Tracker ==="
say "Goal: " + goal

keep going while saved < goal:
    saved = saved + weekly_saving
    weeks = weeks + 1

say "Weeks to reach goal: " + weeks
say "Total saved: " + saved

# Calculate what compound interest would add
remember with_interest = finance.compound(saved, 0.05, 5)
say "If invested at 5% for 5 years: " + with_interest

# Budget the savings
remember budget = finance.budget(saved)
say "---"
say "Budget breakdown:"
say "  Needs:   " + budget[needs]
say "  Savings: " + budget[savings]
say "  Wants:   " + budget[wants]

say "=== Well done! ==="
`,
        expected: '=== Savings Tracker ===\nGoal: 1000\nWeeks to reach goal: 20\nTotal saved: 1000\nIf invested at 5% for 5 years: 1276.28\n---\nBudget breakdown:\n  Needs:   500\n  Savings: 300\n  Wants:   200\n=== Well done! ==='
    },
    // ═══════════ TIER 2 ADVANCED (Lessons 21-30) ═══════════
    {
        id: 21,
        title: "Finding and Sorting",
        subtitle: "Search and sort algorithms",
        concept: "algorithms",
        body: `
<p>How do you find something in a bag of a million items? How do you sort them? These are the fundamental questions of computer science.</p>
<div class="hint">Efficiency matters. A good algorithm can search a billion items in 30 steps. A bad one takes a billion steps. This is the difference between a fast app and a slow one.</div>
<p>ILMA gives you <code>.sorted()</code> built in, but let's understand HOW sorting works by building our own!</p>
`,
        code: `# Finding and Sorting
remember numbers = bag[64, 25, 12, 22, 11]

# Built-in sort
remember sorted_nums = numbers.sorted()
say "Sorted: "
for each n in sorted_nums:
    say "  " + n

# Let's build a search recipe
recipe find_in(items, target):
    remember index = 0
    for each item in items:
        if item is target:
            say "Found " + target + " at position " + index
            give back index
        index = index + 1
    say target + " not found"
    give back -1

say "---"
remember pos = find_in(sorted_nums, 22)
remember pos2 = find_in(sorted_nums, 99)
`,
        expected: 'Sorted: \n  11\n  12\n  22\n  25\n  64\n---\nFound 22 at position 2\n99 not found'
    },
    {
        id: 22,
        title: "Drawing with Code",
        subtitle: "Islamic geometric art",
        concept: "use draw",
        body: `
<p>Islamic art is famous for its stunning geometric patterns — stars, tessellations, and symmetry. These patterns are all mathematics!</p>
<div class="hint">Islamic geometric art uses no images of living beings. Instead, it celebrates the infinite through mathematics and symmetry. Every pattern is a meditation on the perfection of creation.</div>
<p>In this lesson we'll create patterns using maths and text art.</p>
`,
        code: `# Drawing patterns with code
# Text-based art (canvas module coming soon!)

recipe draw_triangle(size):
    remember row = 1
    keep going while row <= size:
        remember spaces = ""
        remember stars = ""
        remember s = 0
        keep going while s < size - row:
            spaces = spaces + " "
            s = s + 1
        remember t = 0
        keep going while t < 2 * row - 1:
            stars = stars + "*"
            t = t + 1
        say spaces + stars
        row = row + 1

draw_triangle(5)
say "---"

# Star pattern
recipe draw_diamond(size):
    remember i = 1
    keep going while i <= size:
        remember spaces = ""
        remember stars = ""
        remember s = 0
        keep going while s < size - i:
            spaces = spaces + " "
            s = s + 1
        remember t = 0
        keep going while t < 2 * i - 1:
            stars = stars + "*"
            t = t + 1
        say spaces + stars
        i = i + 1
    i = size - 1
    keep going while i >= 1:
        remember spaces2 = ""
        remember stars2 = ""
        remember s2 = 0
        keep going while s2 < size - i:
            spaces2 = spaces2 + " "
            s2 = s2 + 1
        remember t2 = 0
        keep going while t2 < 2 * i - 1:
            stars2 = stars2 + "*"
            t2 = t2 + 1
        say spaces2 + stars2
        i = i - 1

draw_diamond(4)
`,
        expected: '    *\n   ***\n  *****\n *******\n*********\n---\n   *\n  ***\n *****\n*******\n *****\n  ***\n   *'
    },
    {
        id: 23,
        title: "My First Game",
        subtitle: "Combining all Tier 2 concepts",
        concept: "game logic",
        body: `
<p>Games have <strong>rules</strong>, <strong>states</strong>, and <strong>goals</strong> — just like a well-lived life.</p>
<p>Let's build a number guessing game that uses everything you've learned!</p>
<div class="hint">A game is a program with a loop, decisions, and a goal. Every app you use — from Instagram to a banking app — follows the same pattern.</div>
`,
        code: `# Number Guessing Game (automated demo)
# In a real game, you'd use ask to get player input

recipe play_game(secret, guesses):
    remember attempts = 0
    for each guess in guesses:
        attempts = attempts + 1
        if guess is secret:
            say "Guess " + attempts + ": " + guess + " — Correct!"
            give back attempts
        otherwise if guess < secret:
            say "Guess " + attempts + ": " + guess + " — Too low!"
        otherwise:
            say "Guess " + attempts + ": " + guess + " — Too high!"
    say "Out of guesses!"
    give back -1

say "=== Number Guessing Game ==="
say "I'm thinking of a number between 1 and 100..."
say ""

remember result = play_game(42, bag[50, 25, 37, 44, 42])

if result > 0:
    say ""
    say "You won in " + result + " guesses!"
otherwise:
    say "You didn't guess it!"
`,
        expected: '=== Number Guessing Game ===\nI\'m thinking of a number between 1 and 100...\n\nGuess 1: 50 — Too high!\nGuess 2: 25 — Too low!\nGuess 3: 37 — Too low!\nGuess 4: 44 — Too high!\nGuess 5: 42 — Correct!\n\nYou won in 5 guesses!'
    },
    {
        id: 24,
        title: "The Quran Module",
        subtitle: "Sacred text in code",
        concept: "use quran",
        body: `
<p>The <code>quran</code> module lets you explore ayat (verses) of the Quran through code.</p>
<div class="hint">The first word revealed to Prophet Muhammad (peace be upon him) was <strong>Iqra</strong> — Read. Seeking knowledge is worship. This module lets you search, reflect, and learn.</div>
<p>Functions: <code>quran.ayah_of_the_day()</code>, <code>quran.search()</code>, <code>quran.surah()</code></p>
`,
        code: `# The Quran Module
use quran

# Look up a surah
remember ikhlas = quran.surah("Al-Ikhlas")
say "Surah: " + ikhlas[surah]
say "Arabic: " + ikhlas[arabic]
say "Translation: " + ikhlas[translation]

say "---"

# Search for a word
remember results = quran.search("Lord")
say "Ayat mentioning 'Lord': " + results.size

for each ayah in results:
    say "  " + ayah[surah] + ": " + ayah[translation]
`,
        expected: 'Surah: Al-Ikhlas\nArabic: قُلْ هُوَ اللَّهُ أَحَدٌ\nTranslation: Say: He is Allah, the One\n---\nAyat mentioning \'Lord\': 2\n  Al-Alaq: Read in the name of your Lord who created\n  Ar-Rahman: So which of the favours of your Lord would you deny?'
    },
    {
        id: 25,
        title: "Trade and Markets",
        subtitle: "The ethics of commerce",
        concept: "use trade",
        body: `
<p>The <code>trade</code> module teaches economics: profit, supply and demand, and halal commerce.</p>
<div class="hint">In Islam, trade is honourable. The Prophet (peace be upon him) was a trader. But trade must be fair: no deception, no interest (riba), no gambling (maysir).</div>
`,
        code: `# Trade and Markets
use trade

# Profit calculation
remember cost = 200
remember revenue = 340
say "Cost: " + cost
say "Revenue: " + revenue
say "Profit: " + trade.profit(cost, revenue)
say "Margin: " + trade.margin(cost, revenue) + "%"

say "---"

# Halal trade checker
remember check1 = trade.halal_check(no, no)
say "Fair trade: " + check1[reason]

remember check2 = trade.halal_check(yes, no)
say "With interest: " + check2[reason]

say "---"

# Supply and demand
remember sim = trade.supply_demand("dates", 20)
say sim[product] + " price after +20% demand: " + sim[new_price]
`,
        expected: 'Cost: 200\nRevenue: 340\nProfit: 140\nMargin: 41.18%\n---\nFair trade: This trade is halal\nWith interest: This trade involves prohibited elements\n---\ndates price after +20% demand: 12'
    },
    {
        id: 26,
        title: "Science Experiments",
        subtitle: "Physics in code",
        concept: "use science",
        body: `
<p>The <code>science</code> module lets you simulate physics experiments: gravity, energy, temperature.</p>
<div class="hint">Islam and science go hand in hand. Muslim scholars like Ibn al-Haytham (optics), Al-Khwarizmi (algebra), and Jabir ibn Hayyan (chemistry) laid the foundations of modern science.</div>
`,
        code: `# Science Experiments
use science

# How long does it take to fall?
say "=== Gravity ==="
remember height = 100
remember fall_time = science.gravity_fall(height)
say "Drop from " + height + "m: " + fall_time + " seconds"

# Temperature conversion
say "=== Temperature ==="
say "100°C = " + science.celsius_to_fahrenheit(100) + "°F"
say "32°F = " + science.fahrenheit_to_celsius(32) + "°C"

# Kinetic energy
say "=== Energy ==="
remember ke = science.kinetic_energy(10, 20)
say "10kg at 20m/s: " + ke + " Joules"

# Distance
say "=== Distance ==="
remember d = science.distance(60, 2.5)
say "60km/h for 2.5h = " + d + " km"

say "=== Pi ==="
say "Pi = " + science.pi()
`,
        expected: null // pi output varies by platform
    },
    {
        id: 27,
        title: "How Computers Count",
        subtitle: "Binary, hex, and Roman numerals",
        concept: "use number",
        body: `
<p>Humans count in base 10. Computers count in base 2 (binary). Programmers also use base 16 (hexadecimal).</p>
<div class="hint">The Arabic numeral system (0-9) was brought to Europe by Muslim mathematicians. Before that, Europeans used Roman numerals. Al-Khwarizmi — from whose name we get "algorithm" — wrote the book that introduced these numbers to the world.</div>
`,
        code: `# How Computers Count
use number

say "=== Binary (base 2) ==="
say "42 in binary: " + number.to_binary(42)
say "255 in binary: " + number.to_binary(255)
say "Binary 1010 = " + number.from_binary("1010")

say "=== Hexadecimal (base 16) ==="
say "255 in hex: " + number.to_hex(255)
say "Hex FF = " + number.from_hex("FF")

say "=== Roman Numerals ==="
say "2024 in Roman: " + number.to_roman(2024)
say "1453 in Roman: " + number.to_roman(1453)

say "=== Prime Numbers ==="
remember primes = bag[]
remember n = 2
keep going while n <= 30:
    if number.is_prime(n):
        primes.add(n)
    n = n + 1
say "Primes up to 30: " + ", ".join(primes)

say "=== Fibonacci ==="
remember fib = number.fibonacci(10)
say "First 10: " + ", ".join(fib)
`,
        expected: '=== Binary (base 2) ===\n42 in binary: 101010\n255 in binary: 11111111\nBinary 1010 = 10\n=== Hexadecimal (base 16) ===\n255 in hex: FF\nHex FF = 255\n=== Roman Numerals ===\n2024 in Roman: MMXXIV\n1453 in Roman: MCDLIII\n=== Prime Numbers ===\nPrimes up to 30: 2, 3, 5, 7, 11, 13, 17, 19, 23, 29\n=== Fibonacci ===\nFirst 10: 0, 1, 1, 2, 3, 5, 8, 13, 21, 34'
    },
    {
        id: 28,
        title: "Things that Call Themselves",
        subtitle: "Recursion",
        concept: "recursion",
        body: `
<p><strong>Recursion</strong> is when a recipe calls itself. It sounds strange, but it's one of the most powerful ideas in programming.</p>
<div class="hint">Self-reference appears everywhere: mirrors reflecting mirrors, a story within a story, fractals in nature. The Quran says "We will show them Our signs in the horizons and within themselves." Recursion is a sign in code.</div>
<p>Every recursive recipe needs a <strong>base case</strong> — a point where it stops calling itself.</p>
`,
        code: `# Recursion — things that call themselves

# Factorial: 5! = 5 × 4 × 3 × 2 × 1
recipe factorial(n):
    if n <= 1:
        give back 1
    give back n * factorial(n - 1)

say "5! = " + factorial(5)
say "10! = " + factorial(10)

# Fibonacci (recursive)
recipe fib(n):
    if n <= 0:
        give back 0
    if n is 1:
        give back 1
    give back fib(n - 1) + fib(n - 2)

say "---"
say "Fibonacci sequence:"
remember i = 0
keep going while i < 10:
    say "  fib(" + i + ") = " + fib(i)
    i = i + 1

# Power function
recipe power(base, exp):
    if exp is 0:
        give back 1
    give back base * power(base, exp - 1)

say "---"
say "2^10 = " + power(2, 10)
`,
        expected: '5! = 120\n10! = 3628800\n---\nFibonacci sequence:\n  fib(0) = 0\n  fib(1) = 1\n  fib(2) = 1\n  fib(3) = 2\n  fib(4) = 3\n  fib(5) = 5\n  fib(6) = 8\n  fib(7) = 13\n  fib(8) = 21\n  fib(9) = 34\n---\n2^10 = 1024'
    },
    {
        id: 29,
        title: "My Capstone Project",
        subtitle: "Build something real",
        concept: "capstone",
        body: `
<p>This is your <strong>capstone project</strong>. It combines everything: variables, decisions, loops, bags, recipes, notebooks, modules, and error handling.</p>
<p>This example builds a complete <strong>student report card system</strong>.</p>
<div class="hint">Identify a problem in your family or community. Solve it with ILMA. That is the true purpose of programming — not to impress, but to help.</div>
<p><strong>Challenge:</strong> Modify this to track YOUR school subjects!</p>
`,
        code: `# Student Report Card System
use finance

recipe calculate_grade(score):
    if score >= 90:
        give back "A"
    otherwise if score >= 80:
        give back "B"
    otherwise if score >= 70:
        give back "C"
    otherwise if score >= 60:
        give back "D"
    give back "F"

recipe generate_report(name, scores):
    say "=== Report Card: " + name + " ==="
    remember total = 0
    remember count = 0
    for each subject in scores:
        remember score = scores[(subject)]
        remember grade = calculate_grade(score)
        say "  " + subject + ": " + score + " (" + grade + ")"
        total = total + score
        count = count + 1
    remember average = total / count
    say "  ---"
    say "  Average: " + average + " (" + calculate_grade(average) + ")"
    give back average

remember student1 = notebook[maths: 92, science: 88, english: 75, arabic: 95, history: 82]
remember avg1 = generate_report("Amira", student1)

say ""

remember student2 = notebook[maths: 78, science: 65, english: 90, arabic: 85, history: 70]
remember avg2 = generate_report("Yusuf", student2)

say ""
say "=== Class Summary ==="
if avg1 > avg2:
    say "Amira has the higher average"
otherwise:
    say "Yusuf has the higher average"
`,
        expected: null // notebook iteration order varies
    },
    {
        id: 30,
        title: "Teaching Someone Else",
        subtitle: "True mastery is the ability to teach",
        concept: "documentation",
        body: `
<p>You have completed the <strong>entire ILMA Tier 2 curriculum</strong>. You now know: variables, decisions, loops, bags, notebooks, recipes, error handling, modules, recursion, and algorithms.</p>

<p>The final lesson: <strong>teach what you know</strong>.</p>

<div class="hint">The Prophet (peace be upon him) said: "The best of you are those who learn the Quran and teach it." The same applies to knowledge. True mastery is being able to explain your code to a younger student.</div>

<p>This program is fully commented — every line explains what it does and why. This is how you should write code: clear enough that a beginner can follow.</p>
`,
        code: `# Teaching Someone Else
# This program demonstrates every major ILMA concept
# with clear comments explaining each one.

# === VARIABLES ===
# 'remember' stores a value with a name
remember teacher = "You"
remember students = 3

# === OUTPUT ===
# 'say' prints text to the screen
say teacher + " will teach " + students + " students today"

# === DECISIONS ===
# 'if' checks a condition and runs code based on the result
if students > 0:
    say "Let's begin!"

# === LOOPS ===
# 'repeat' does something a fixed number of times
repeat students:
    say "  Welcome, student!"

# === BAGS (LISTS) ===
# A bag holds a collection of items in order
remember topics = bag["variables", "loops", "recipes", "bags"]
say "---"
say "Today we will cover:"
for each topic in topics:
    say "  - " + topic

# === RECIPES (FUNCTIONS) ===
# A recipe is a reusable skill
recipe explain(concept):
    say "Let me explain: " + concept
    say "  It's simpler than you think!"

say "---"
explain("recursion")

# === ERROR HANDLING ===
# try/when wrong catches mistakes gracefully
try:
    shout "This is a test error"
when wrong:
    say "---"
    say "Errors are okay — we learn from them!"

# === CONCLUSION ===
say "---"
say "You now know ILMA!"
say "Go build something amazing."
say "Bismillah."
`,
        expected: 'You will teach 3 students today\nLet\'s begin!\n  Welcome, student!\n  Welcome, student!\n  Welcome, student!\n---\nToday we will cover:\n  - variables\n  - loops\n  - recipes\n  - bags\n---\nLet me explain: recursion\n  It\'s simpler than you think!\n---\nErrors are okay — we learn from them!\n---\nYou now know ILMA!\nGo build something amazing.\nBismillah.'
    }
];
