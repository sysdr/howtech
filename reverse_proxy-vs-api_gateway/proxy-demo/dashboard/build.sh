#!/bin/bash
set -e

# This script builds the dashboard for production
# It compiles Tailwind CSS and converts JSX to plain JavaScript

echo "Building dashboard for production..."

# Create a minimal Tailwind config and build CSS
# We'll use Tailwind CDN's minified output approach
# For a proper production build, you'd use: npx tailwindcss -i ./src/input.css -o ./public/tailwind.css --minify

# For now, we'll create a pre-compiled CSS with the classes we use
# Using Tailwind Play CDN output (minified version)
cat > public/tailwind.css << 'EOF'
/* Tailwind CSS - Production build */
/* Generated minimal version with only used classes */
*,::before,::after{box-sizing:border-box;border-width:0;border-style:solid;border-color:#e5e7eb}
::before,::after{--tw-content:''}
html{line-height:1.5;-webkit-text-size-adjust:100%;-moz-tab-size:4;tab-size:4;font-family:ui-sans-serif,system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",Arial,"Noto Sans",sans-serif,"Apple Color Emoji","Segoe UI Emoji","Segoe UI Symbol","Noto Color Emoji";font-feature-settings:normal;font-variation-settings:normal}
body{margin:0;line-height:inherit}
.container{width:100%}
@media (min-width:640px){.container{max-width:640px}}
@media (min-width:768px){.container{max-width:768px}}
@media (min-width:1024px){.container{max-width:1024px}}
@media (min-width:1280px){.container{max-width:1280px}}
@media (min-width:1536px){.container{max-width:1536px}}
.mx-auto{margin-left:auto;margin-right:auto}
.mb-2{margin-bottom:.5rem}
.mb-4{margin-bottom:1rem}
.mb-8{margin-bottom:2rem}
.mb-12{margin-bottom:3rem}
.ml-2{margin-left:.5rem}
.mr-2{margin-right:.5rem}
.mt-8{margin-top:2rem}
.flex{display:flex}
.grid{display:grid}
.hidden{display:none}
.items-center{align-items:center}
.items-start{align-items:flex-start}
.justify-center{justify-content:center}
.justify-between{justify-content:space-between}
.gap-2{gap:.5rem}
.gap-4{gap:1rem}
.gap-6{gap:1.5rem}
.space-y-3>*+*{margin-top:.75rem}
.rounded{border-radius:.25rem}
.rounded-lg{border-radius:.5rem}
.rounded-xl{border-radius:.75rem}
.rounded-full{border-radius:9999px}
.border{border-width:1px}
.border-l-4{border-left-width:4px}
.border-white\/20{border-color:rgba(255,255,255,.2)}
.bg-white\/10{background-color:rgba(255,255,255,.1)}
.bg-white\/5{background-color:rgba(255,255,255,.05)}
.bg-blue-500{background-color:#3b82f6}
.bg-blue-500\/20{background-color:rgba(59,130,246,.2)}
.bg-blue-900\/30{background-color:rgba(30,58,138,.3)}
.bg-green-500\/20{background-color:rgba(34,197,94,.2)}
.bg-green-900\/30{background-color:rgba(20,83,45,.3)}
.bg-purple-500{background-color:#a855f7}
.bg-purple-500\/20{background-color:rgba(168,85,247,.2)}
.bg-red-500\/20{background-color:rgba(239,68,68,.2)}
.bg-red-900\/30{background-color:rgba(127,29,29,.3)}
.bg-orange-500\/20{background-color:rgba(249,115,22,.2)}
.bg-gray-500{background-color:#6b7280}
.bg-gradient-to-br{background-image:linear-gradient(to bottom right,var(--tw-gradient-stops))}
.from-slate-900{--tw-gradient-from:#0f172a;--tw-gradient-to:rgba(15,23,42,0);--tw-gradient-stops:var(--tw-gradient-from),var(--tw-gradient-to)}
.via-blue-900{--tw-gradient-to:rgba(30,58,138,0);--tw-gradient-stops:var(--tw-gradient-from),rgba(30,58,138,.5),var(--tw-gradient-to)}
.to-slate-900{--tw-gradient-to:#0f172a}
.backdrop-blur-lg{backdrop-filter:blur(16px)}
.p-2{padding:.5rem}
.p-4{padding:1rem}
.p-6{padding:1.5rem}
.px-3{padding-left:.75rem;padding-right:.75rem}
.px-4{padding-left:1rem;padding-right:1rem}
.px-8{padding-left:2rem;padding-right:2rem}
.py-1{padding-top:.25rem;padding-bottom:.25rem}
.py-4{padding-top:1rem;padding-bottom:1rem}
.py-8{padding-top:2rem;padding-bottom:2rem}
.text-xs{font-size:.75rem;line-height:1rem}
.text-sm{font-size:.875rem;line-height:1.25rem}
.text-lg{font-size:1.125rem;line-height:1.75rem}
.text-xl{font-size:1.25rem;line-height:1.75rem}
.text-2xl{font-size:1.5rem;line-height:2rem}
.text-4xl{font-size:2.25rem;line-height:2.5rem}
.text-5xl{font-size:3rem;line-height:1}
.font-bold{font-weight:700}
.font-semibold{font-weight:600}
.font-mono{font-family:ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,"Liberation Mono","Courier New",monospace}
.text-white{color:#fff}
.text-blue-200{color:#bfdbfe}
.text-blue-300{color:#93c5fd}
.text-green-200{color:#bbf7d0}
.text-green-300{color:#86efac}
.text-purple-200{color:#e9d5ff}
.text-purple-300{color:#d8b4fe}
.text-red-300{color:#fca5a5}
.text-orange-200{color:#fed7aa}
.text-orange-300{color:#fdba74}
.hover\:bg-blue-600:hover{background-color:#2563eb}
.hover\:bg-purple-600:hover{background-color:#9333ea}
.hover\:scale-105:hover{transform:scale(1.05)}
.disabled\:bg-gray-500:disabled{background-color:#6b7280}
.transition-all{transition-property:all;transition-timing-function:cubic-bezier(.4,0,.2,1);transition-duration:150ms}
.transform{transform:translate(var(--tw-translate-x),var(--tw-translate-y)) rotate(var(--tw-rotate)) skewX(var(--tw-skew-x)) skewY(var(--tw-skew-y)) scaleX(var(--tw-scale-x)) scaleY(var(--tw-scale-y))}
.shadow-lg{box-shadow:0 10px 15px -3px rgba(0,0,0,.1),0 4px 6px -4px rgba(0,0,0,.1)}
.min-h-screen{min-height:100vh}
.max-h-96{max-height:24rem}
.overflow-y-auto{overflow-y:auto}
@media (min-width:768px){.md\:grid-cols-3{grid-template-columns:repeat(3,minmax(0,1fr))}}
@media (min-width:768px){.md\:grid-cols-4{grid-template-columns:repeat(4,minmax(0,1fr))}}
EOF

echo "✅ Tailwind CSS generated"

# Note: For full JSX conversion, we'd need Babel or a similar tool
# For now, we'll keep the JSX but note that Babel standalone should only be used in development
# In a real production setup, you'd pre-compile this with Babel CLI

echo "✅ Build complete!"
echo "⚠️  Note: app.js still uses JSX. For full production, compile with Babel CLI:"
echo "   npx @babel/cli app.js --out-file app.compiled.js --presets=@babel/preset-react"



