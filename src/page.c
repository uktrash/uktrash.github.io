#include "page.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct system_fact {
  const char *label;
  const char *value;
};

struct dossier_metric {
  const char *value;
  const char *label;
};

struct dossier_step {
  const char *index;
  const char *label;
  const char *detail;
};

struct dossier {
  const char *title;
  const char *stamp;
  const char *lede;
  struct dossier_step steps[8];
  size_t step_count;
  struct dossier_metric metrics[4];
  size_t metric_count;
  const char *limits;
};

struct project {
  const char *name;
  const char *stack;
  const char *focus;
  const char *tradeoff;
  const char *evidence;
  const char *repo;
  bool interactive;
  const char *cta_label;
  const char *dossier_id;
};

struct dossier_entry {
  const char *prefix;
  const struct dossier *dossier;
};

static const struct system_fact SYSTEM_FACTS[] = {
    {"Runtime", "Raw C"},
    {"Serving", "Event-driven HTTP"},
    {"Latency", "51 us p50 / 118 us p99 local loopback"},
};

static const struct dossier ES_DOSSIER = {
    "ES Microedge Study",
    "CME ES, Jun 2010-Apr 2025",
    "Question: whether session segmentation and a low-range state variable explain a stable intraday mean-reversion pattern in ES, with the claim kept inside explicit proxy assumptions.",
    {
        {"01", "Question",
         "Conditional on a compressed Asia range, do one-minute ES shocks mean-revert once the reopen disturbance has cleared?"},
        {"02", "Sample",
         "5,194,387 one-minute ES bars from June 6, 2010 through April 25, 2025, labeled on an 18:00 ET trading day."},
        {"03", "Construction",
         "For each minute t, select the highest-volume outright contract. Exclude spreads and synthetics, then stitch the outright into a continuous proxy series."},
        {"04", "State Variable",
         "Define consolidation from Asia-session range scaled by realized variation. The filter retains 1,528 of 3,820 days with an Asia session."},
        {"05", "Reopen Diagnostic",
         "Mean absolute movement in the first 10 reopen minutes is 28.45 ticks versus 2.23 ticks in minutes 20-30, a 12.76x shock ratio."},
        {"06", "Europe-Open Tail",
         "With a 23-tick absolute-return threshold, the Europe-open tail rate is 1.23x the rate observed outside the Europe-open window."},
        {"07", "Candidate Rule",
         "On consolidation days only, fade +/-2 tick one-minute shocks during Asia; skip the first 10 reopen minutes; hold for 5 minutes."},
        {"08", "Proxy Result",
         "25,678 proxy trades, 56.15% hit rate, average win 12.36 ticks, average loss -3.10 ticks, gross EV +5.58 ticks per trade, max drawdown 400 ticks."},
    },
    8,
    {
        {"5.19M", "1-minute bars"},
        {"25,678", "Proxy trades"},
        {"56.15%", "Hit rate"},
        {"+5.58 ticks", "Gross EV / trade"},
    },
    4,
    "Statistical evidence only. 1-minute OHLCV omits queue position, intrabar path, fees, slippage, and impact; no live execution claim follows from this note.",
};

static const struct dossier SITE_DOSSIER = {
    "Implementation Notes",
    "Portfolio site, Mar 2026",
    "Static portfolio generated in C, served through a small poll-based HTTP loop, and exported directly for GitHub Pages.",
    {
        {"01", "Surface",
         "Single-page portfolio with a restrained visual layer and two typefaces. The public surface stays short so the code and notes carry the technical weight."},
        {"02", "Serving path",
         "HTTP server implemented in raw C with a poll-based event loop. Connections are read incrementally, routed with a minimal parser, and flushed without framework middleware."},
        {"03", "Response construction",
         "The homepage HTML is assembled in C and the primary response is prebuilt once at startup, reducing per-request work to routing and socket I/O."},
        {"04", "Export model",
         "The same renderer exports static HTML for GitHub Pages, so the deployed surface remains identical whether served live or published statically."},
        {"05", "Benchmark method",
         "Latency figures are measured locally on loopback and reported as local measurements only. They characterize the response path and do not imply network or production performance."},
        {"06", "Omissions",
         "No framework, no runtime dependency chain, no client-side bundle, and no analytics layer. Each omission reduces moving parts and makes behavior easier to inspect."},
    },
    6,
    {
        {"Raw C", "Renderer + server"},
        {"poll", "Event loop"},
        {"51 us", "p50 local"},
        {"118 us", "p99 local"},
    },
    4,
    "Local loopback measurements only. Figures exclude WAN latency, TLS, browser rendering, CDN effects, and production contention.",
};

static const struct dossier_entry DOSSIERS[] = {
    {"es", &ES_DOSSIER},
    {"site", &SITE_DOSSIER},
};

static const struct project PROJECTS[] = {
    {"ES Microedge Study",
     "Python, futures data research, session modeling, backtesting",
     "Intraday ES research note on reopen dislocation, Europe-open tail behavior, and conditioned mean reversion.",
     "1-minute OHLCV proxy; session segmentation and a consolidation filter are used to bound the claim.",
     "5.19M bars, 25,678 proxy trades, 56.15% hit rate, +5.58 ticks gross EV.",
     "",
     true,
     "Read Note",
     "es-dossier"},
    {"Blockchain Consensus Simulator",
     "Python, SHA-256, sockets, Docker",
     "Multi-node proof-of-work simulator for validation, propagation, and peer synchronization.",
     "Socket-level coordination and containerized repeatability are used instead of managed network abstractions.",
     "Deterministic multi-node runs verify block validation, propagation order, and state convergence.",
     "",
     false,
     NULL,
      NULL},
    {"Financial Return Forecasting",
     "Python, TensorFlow, GRU/LSTM",
     "Rolling-window return forecasting pipeline for recurrent models under changing market regimes.",
     "Benchmarked against simpler baselines to separate model fit from non-stationary drift.",
     "Repeated retraining windows compare forecast behavior and stability across samples.",
     "",
     false,
     NULL,
      NULL},
};

static const size_t SYSTEM_FACT_COUNT = sizeof(SYSTEM_FACTS) / sizeof(SYSTEM_FACTS[0]);
static const size_t DOSSIER_COUNT = sizeof(DOSSIERS) / sizeof(DOSSIERS[0]);
static const size_t PROJECT_COUNT = sizeof(PROJECTS) / sizeof(PROJECTS[0]);

static size_t appendf(char *out, size_t cap, size_t used, const char *fmt, ...) {
  if (used >= cap) {
    return cap;
  }

  va_list ap;
  va_start(ap, fmt);
  int written = vsnprintf(out + used, cap - used, fmt, ap);
  va_end(ap);
  if (written < 0) {
    return cap;
  }

  size_t w = (size_t)written;
  if (w >= cap - used) {
    return cap;
  }
  return used + w;
}

static bool has_text(const char *text) {
  return text != NULL && text[0] != '\0';
}

static size_t append_system_fact(
    char *out,
    size_t cap,
    size_t used,
    const struct system_fact *fact) {
  return appendf(
      out, cap, used,
      "<div class=\"system-fact\">"
      "<span class=\"system-label\">%s</span>"
      "<span class=\"system-value\">%s</span>"
      "</div>",
      fact->label, fact->value);
}

static size_t append_project_card(
    char *out,
    size_t cap,
    size_t used,
    const struct project *project,
    size_t index) {
  const char *card_class = project->interactive ? " project-card-interactive" : "";

  used = appendf(
      out, cap, used,
      "<article class=\"project-card%s\">"
      "<div class=\"project-top\">"
      "<div>"
      "<div class=\"project-index\">%02zu</div>"
      "<h3 class=\"project-title\">%s</h3>"
      "</div>"
      "<p class=\"project-meta\">%s</p>"
      "</div>"
      "<div class=\"project-lines\">"
      "<p class=\"project-record\"><span class=\"project-label\">System</span><span class=\"project-copy\">%s</span></p>"
      "<p class=\"project-record\"><span class=\"project-label\">Tradeoff</span><span class=\"project-copy\">%s</span></p>"
      "<p class=\"project-record\"><span class=\"project-label\">Evidence</span><span class=\"project-copy\">%s</span></p>"
      "</div>",
      card_class, index + 1, project->name, project->stack, project->focus,
      project->tradeoff, project->evidence);

  if (project->interactive && has_text(project->dossier_id) && has_text(project->cta_label)) {
    used = appendf(
        out, cap, used,
        "<button class=\"project-action dossier-trigger\" type=\"button\" "
        "aria-haspopup=\"dialog\" aria-controls=\"%s\" aria-expanded=\"false\">%s</button>",
        project->dossier_id, project->cta_label);
  } else if (has_text(project->repo)) {
    used = appendf(
        out, cap, used,
        "<a class=\"project-action\" href=\"%s\" rel=\"noopener noreferrer\">Repository</a>",
        project->repo);
  } else {
    used = appendf(out, cap, used,
                   "<span class=\"project-action project-action-static\">Private</span>");
  }

  used = appendf(out, cap, used, "</article>");
  return used;
}

static size_t append_dossier_overlay(
    char *out,
    size_t cap,
    size_t used,
    const char *prefix,
    const struct dossier *dossier) {
  if (dossier == NULL) {
    return used;
  }

  used = appendf(
      out, cap, used,
      "<div class=\"dossier-layer\" id=\"%s-dossier-layer\" hidden>"
      "<div class=\"dossier-backdrop\" data-dossier-close></div>"
      "<section class=\"dossier-shell\" id=\"%s-dossier\" role=\"dialog\" aria-modal=\"true\" "
      "aria-labelledby=\"%s-dossier-title\">"
      "<div class=\"dossier-content\">"
      "<div class=\"dossier-head\">"
      "<p class=\"dossier-label\">Technical note</p>"
      "<button class=\"dossier-close\" type=\"button\" data-dossier-close>Close</button>"
      "</div>"
      "<p class=\"dossier-stamp\">%s</p>"
      "<h2 class=\"dossier-title\" id=\"%s-dossier-title\">%s</h2>"
      "<p class=\"dossier-lede\">%s</p>"
      "<div class=\"dossier-body\">"
      "<aside class=\"dossier-column dossier-rail\">"
      "<section class=\"dossier-section\">"
      "<p class=\"dossier-label\">Sections</p>"
      "<ol class=\"dossier-contents\">",
      prefix, prefix, prefix, dossier->stamp, prefix, dossier->title,
      dossier->lede);

  for (size_t i = 0; i < dossier->step_count; ++i) {
    const struct dossier_step *step = &dossier->steps[i];
    used = appendf(
        out, cap, used,
        "<li class=\"dossier-contents-item\">"
        "<a class=\"dossier-toc-link\" href=\"#%s-step-%s\">"
        "<span class=\"dossier-toc-index\">%s</span>"
        "<span class=\"dossier-toc-text\">%s</span>"
        "</a>"
        "</li>",
        prefix, step->index, step->index, step->label);
  }

  used = appendf(
      out, cap, used,
      "</ol>"
      "</section>"
      "<section class=\"dossier-section\">"
      "<p class=\"dossier-label\">Selected statistics</p>"
      "<div class=\"dossier-metrics\">");

  for (size_t i = 0; i < dossier->metric_count; ++i) {
    const struct dossier_metric *metric = &dossier->metrics[i];
    used = appendf(
        out, cap, used,
        "<div class=\"dossier-metric\">"
        "<span class=\"dossier-metric-value\">%s</span>"
        "<span class=\"dossier-metric-label\">%s</span>"
        "</div>",
        metric->value, metric->label);
  }

  used = appendf(
      out, cap, used,
      "</div>"
      "</section>"
      "<section class=\"dossier-section\">"
      "<p class=\"dossier-label\">Model risk</p>"
      "<p class=\"dossier-copy dossier-note\">%s</p>"
      "</section>"
      "</aside>"
      "<div class=\"dossier-column dossier-paper\">",
      dossier->limits);

  for (size_t i = 0; i < dossier->step_count; ++i) {
    const struct dossier_step *step = &dossier->steps[i];
    used = appendf(
        out, cap, used,
        "<section class=\"dossier-section dossier-paper-section\" id=\"%s-step-%s\">"
        "<div class=\"dossier-paper-head\">"
        "<div class=\"dossier-step-index\">%s</div>"
        "<p class=\"dossier-step-label\">%s</p>"
        "</div>"
        "<p class=\"dossier-copy\">%s</p>"
        "</section>",
        prefix, step->index, step->index, step->label, step->detail);
  }

  used = appendf(
      out, cap, used,
      "</div>"
      "</div>"
      "</div>"
      "</section>"
      "</div>");

  return used;
}

static size_t append_dossier_script(char *out, size_t cap, size_t used) {
  used = appendf(
      out, cap, used,
      "<script>"
      "(function(){"
      "const main=document.querySelector('.site-main');"
      "const triggers=Array.from(document.querySelectorAll('.dossier-trigger'));"
      "const layers=Array.from(document.querySelectorAll('.dossier-layer'));"
      "const reduced=window.matchMedia('(prefers-reduced-motion: reduce)');"
      "if(!main||!triggers.length||!layers.length){return;}"
      "function targetRect(){"
      "const compact=window.innerWidth<=860;"
      "const margin=compact?8:24;"
      "const top=compact?8:Math.max(32,Math.round(window.innerHeight*0.08));"
      "const bottom=compact?8:Math.max(32,Math.round(window.innerHeight*0.08));"
      "const width=Math.min(960,window.innerWidth-margin*2);"
      "const height=Math.max(320,window.innerHeight-top-bottom);"
      "const left=Math.round((window.innerWidth-width)/2);"
      "return{left:left,top:top,width:width,height:height};"
      "}"
      "function triggerRect(trigger){"
      "const rect=trigger?trigger.getBoundingClientRect():null;"
      "if(!rect){return{left:24,top:24,width:120,height:32};}"
      "return{left:rect.left,top:rect.top,width:rect.width,height:rect.height};"
      "}"
      "function applyTargetRect(){"
      "const rect=targetRect();"
      "shell.style.left=rect.left+'px';"
      "shell.style.top=rect.top+'px';"
      "shell.style.width=rect.width+'px';"
      "shell.style.height=rect.height+'px';"
      "return rect;"
      "}"
      "function motionFrom(origin,target){"
      "const dx=origin.left-target.left;"
      "const dy=origin.top-target.top;"
      "const sx=Math.max(origin.width/target.width,0.08);"
      "const sy=Math.max(origin.height/target.height,0.06);"
      "return 'translate('+dx+'px,'+dy+'px) scale('+sx+','+sy+')';"
      "}"
      "function bindLayer(layer){"
      "const shell=layer.querySelector('.dossier-shell');"
      "const content=layer.querySelector('.dossier-content');"
      "const closeButton=layer.querySelector('.dossier-close');"
      "const backdrop=layer.querySelector('.dossier-backdrop');"
      "const shellId=shell?shell.id:'';"
      "const tocLinks=Array.from(layer.querySelectorAll('.dossier-toc-link'));"
      "const paperSections=Array.from(layer.querySelectorAll('.dossier-paper-section'));"
      "const layerTriggers=triggers.filter(function(trigger){return trigger.getAttribute('aria-controls')===shellId;});"
      "let lastTrigger=null;"
      "let lastOrigin=null;"
      "let settleTimer=0;"
      "let collapseTimer=0;"
      "let closeTimer=0;"
      "let focusTimer=0;"
      "if(!shell||!content||!closeButton||!backdrop||!shellId||!layerTriggers.length){return;}");

  used = appendf(
      out, cap, used,
      "function applyTargetRect(){"
      "const rect=targetRect();"
      "shell.style.left=rect.left+'px';"
      "shell.style.top=rect.top+'px';"
      "shell.style.width=rect.width+'px';"
      "shell.style.height=rect.height+'px';"
      "return rect;"
      "}"
      "function clearCue(){"
      "clearTimeout(focusTimer);"
      "tocLinks.forEach(function(link){link.classList.remove('is-selected');});"
      "paperSections.forEach(function(section){section.classList.remove('is-targeted');});"
      "}"
      "function cueSection(link,section){"
      "clearCue();"
      "if(link){link.classList.add('is-selected');}"
      "section.classList.add('is-targeted');"
      "focusTimer=window.setTimeout(function(){"
      "if(link){link.classList.remove('is-selected');}"
      "section.classList.remove('is-targeted');"
      "},1280);"
      "}"
      "function scrollToSection(section){"
      "const contentRect=content.getBoundingClientRect();"
      "const sectionRect=section.getBoundingClientRect();"
      "const top=Math.max(0,content.scrollTop+sectionRect.top-contentRect.top-18);"
      "if(reduced.matches){content.scrollTop=top;return;}"
      "content.scrollTo({top:top,behavior:'smooth'});"
      "}"
      "function finishClose(){"
      "clearCue();"
      "clearTimeout(settleTimer);"
      "clearTimeout(collapseTimer);"
      "clearTimeout(closeTimer);"
      "layer.classList.remove('is-live');"
      "layer.classList.remove('is-open');"
      "layer.classList.remove('is-settled');"
      "document.body.classList.remove('dossier-open');"
      "main.removeAttribute('inert');"
      "layer.hidden=true;"
      "shell.style.transition='none';"
      "shell.style.transform='none';"
      "shell.style.opacity='';"
      "lastOrigin=null;"
      "if(lastTrigger){lastTrigger.focus();lastTrigger.setAttribute('aria-expanded','false');}"
      "}"
      "function openDossier(trigger){"
      "if(!layer.hidden){return;}"
      "lastTrigger=trigger;"
      "lastOrigin=triggerRect(trigger);"
      "clearTimeout(settleTimer);"
      "clearTimeout(collapseTimer);"
      "clearTimeout(closeTimer);"
      "clearCue();"
      "layer.hidden=false;"
      "layer.classList.add('is-live');"
      "document.body.classList.add('dossier-open');"
      "main.setAttribute('inert','');"
      "content.scrollTop=0;"
      "trigger.setAttribute('aria-expanded','true');"
      "applyTargetRect();"
      "const origin=lastOrigin;"
      "const target=shell.getBoundingClientRect();"
      "shell.style.transition='none';"
      "if(reduced.matches){"
      "layer.classList.add('is-open');"
      "layer.classList.add('is-settled');"
      "shell.style.transform='none';"
      "shell.style.opacity='1';"
      "closeButton.focus();"
      "return;"
      "}"
      "shell.style.transform=motionFrom(origin,target);"
      "shell.style.opacity='0.98';"
      "void shell.offsetHeight;"
      "layer.classList.add('is-open');"
      "requestAnimationFrame(function(){"
      "shell.style.transition='transform 290ms cubic-bezier(0.16,0.84,0.22,1),opacity 220ms linear';"
      "shell.style.transform='none';"
      "shell.style.opacity='1';"
      "});"
      "settleTimer=window.setTimeout(function(){layer.classList.add('is-settled');closeButton.focus();},235);"
      "}"
      "function closeDossier(){"
      "if(layer.hidden){return;}"
      "clearTimeout(settleTimer);"
      "clearTimeout(collapseTimer);"
      "layer.classList.remove('is-settled');"
      "if(reduced.matches){finishClose();return;}"
      "collapseTimer=window.setTimeout(function(){"
      "applyTargetRect();"
      "const target=shell.getBoundingClientRect();"
      "const origin=lastOrigin||triggerRect(lastTrigger);"
      "shell.style.transition='transform 230ms cubic-bezier(0.55,0,0.8,0.2),opacity 170ms linear';"
      "requestAnimationFrame(function(){"
      "shell.style.transform=motionFrom(origin,target);"
      "shell.style.opacity='0.94';"
      "});"
      "closeTimer=window.setTimeout(finishClose,240);"
      "},120);"
      "}");

  used = appendf(
      out, cap, used,
      "layerTriggers.forEach(function(trigger){"
      "trigger.addEventListener('click',function(){openDossier(trigger);});"
      "});"
      "closeButton.addEventListener('click',closeDossier);"
      "backdrop.addEventListener('click',closeDossier);"
      "tocLinks.forEach(function(link){"
      "link.addEventListener('click',function(event){"
      "const href=link.getAttribute('href');"
      "const section=href?shell.querySelector(href):null;"
      "if(!section){return;}"
      "event.preventDefault();"
      "scrollToSection(section);"
      "cueSection(link,section);"
      "});"
      "});"
      "document.addEventListener('keydown',function(event){"
      "if(event.key==='Escape'&&!layer.hidden){event.preventDefault();closeDossier();}"
      "});"
      "window.addEventListener('resize',function(){"
      "if(!layer.hidden){applyTargetRect();}"
      "});"
      "}"
      "layers.forEach(bindLayer);");

  used = appendf(out, cap, used, "})();" "</script>");

  return used;
}

size_t render_portfolio_html(char *out, size_t out_cap) {
  if (out_cap == 0) {
    return 0;
  }

  size_t used = 0;

  used = appendf(
      out, out_cap, used,
      "<!doctype html>"
      "<html lang=\"en\">"
      "<head>"
      "<meta charset=\"utf-8\">"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "<meta name=\"description\" content=\"Systems portfolio by Utkarsh Ambati.\">"
      "<title>Utkarsh Ambati</title>"
      "<link rel=\"icon\" type=\"image/png\" sizes=\"512x512\" href=\"flopper.png\">"
      "<style>"
      "@font-face{font-family:'Bodoni Moda';src:url('fonts/bodoni-moda-400.ttf') format('truetype');font-style:normal;font-weight:400;font-display:swap;}"
      "@font-face{font-family:'Bodoni Moda';src:url('fonts/bodoni-moda-500.ttf') format('truetype');font-style:normal;font-weight:500;font-display:swap;}"
      "@font-face{font-family:'Bodoni Moda';src:url('fonts/bodoni-moda-600.ttf') format('truetype');font-style:normal;font-weight:600;font-display:swap;}"
      "@font-face{font-family:'IBM Plex Mono';src:url('fonts/ibm-plex-mono-400.ttf') format('truetype');font-style:normal;font-weight:400;font-display:swap;}"
      "@font-face{font-family:'IBM Plex Mono';src:url('fonts/ibm-plex-mono-500.ttf') format('truetype');font-style:normal;font-weight:500;font-display:swap;}"
      ":root{--bg:#000000;--panel:#050505;--fg:#f5f5f5;--muted:#8c8c8c;"
      "--accent:#ffffff;--line:rgba(255,255,255,.12);--line-strong:rgba(255,255,255,.3);"
      "--font-display:'Bodoni Moda',Didot,'Times New Roman',serif;"
      "--font-text:'IBM Plex Mono',ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,'Liberation Mono','Courier New',monospace;}"
      "*{box-sizing:border-box}"
      "html{scroll-behavior:smooth}"
      "body{margin:0;padding:0;background:radial-gradient(circle at 50%% -10%%,rgba(255,255,255,.07),transparent 28%%),linear-gradient(180deg,#020202 0%%,var(--bg) 100%%);color:var(--fg);font-family:var(--font-text);font-size:14px;line-height:1.72;letter-spacing:.01em;}"
      "body.dossier-open{overflow:hidden}"
      "body::before{content:'';position:fixed;inset:0;pointer-events:none;background-image:repeating-linear-gradient(180deg,rgba(255,255,255,.018) 0 1px,transparent 1px 4px);mask-image:linear-gradient(180deg,rgba(0,0,0,.5),transparent 76%%);}"
      ".site-main{max-width:1020px;margin:0 auto;padding:44px 22px 88px;position:relative;transition:opacity .18s linear;}"
      "body.dossier-open .site-main{opacity:.25}"
      ".meta,.kicker,.footnote,.system-label,.project-meta,.project-index,.project-label,.project-action,.dossier-label,.dossier-close,.dossier-metric-label,.dossier-toc-index,.dossier-step-index{font-family:var(--font-text);}"
      ".meta{display:flex;justify-content:flex-start;align-items:center;gap:14px;font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);padding-bottom:16px;border-bottom:1px solid var(--line);}"
      ".hero{padding:46px 0 18px;}"
      ".kicker{margin:0 0 12px;font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      "h1{margin:0;max-width:13ch;font-family:var(--font-display);font-size:clamp(2.5rem,5vw,3.8rem);line-height:.97;font-weight:500;letter-spacing:-.03em;color:var(--accent);}"
      ".systems-bar{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px;margin-top:30px;padding-top:14px;border-top:1px solid var(--line);}"
      ".system-fact{padding-top:10px;border-top:1px solid var(--line);}"
      ".system-label{display:block;font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".system-value{display:block;margin-top:8px;color:var(--fg);font-size:.92rem;}"
      ".section-head{display:flex;justify-content:space-between;align-items:end;gap:16px;margin-top:22px;padding-bottom:12px;border-bottom:1px solid var(--line);}"
      ".section-head h2{margin:0;font-family:var(--font-display);font-size:1rem;font-weight:500;letter-spacing:0;color:var(--fg);}"
      ".footnote{font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".projects{display:grid;grid-template-columns:repeat(12,minmax(0,1fr));gap:14px;margin-top:18px;}");

  used = appendf(
      out, out_cap, used,
      ".project-card{grid-column:span 6;padding:16px 16px 15px;background:var(--panel);border:1px solid var(--line);min-height:242px;display:flex;flex-direction:column;justify-content:space-between;position:relative;overflow:hidden;animation:rise .18s cubic-bezier(0.22,0.61,0.36,1) both;animation-delay:.12s;}"
      ".project-card:first-child{grid-column:span 12;min-height:250px;}"
      ".projects .project-card:nth-child(2){animation-delay:.15s;}"
      ".projects .project-card:nth-child(3){animation-delay:.18s;}"
      ".project-card::before{content:'';position:absolute;left:0;right:0;top:0;height:1px;background:linear-gradient(90deg,var(--line-strong),transparent 65%%);}"
      ".project-card::after{content:'';position:absolute;left:16px;right:16px;bottom:0;height:1px;background:linear-gradient(90deg,var(--line-strong),transparent);}"
      ".project-card-interactive:hover{border-color:var(--line-strong);}"
      ".project-top{display:flex;justify-content:space-between;gap:16px;align-items:flex-start;}"
      ".project-index{font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".project-title{margin:4px 0 0;font-family:var(--font-display);font-size:1.22rem;line-height:1.04;font-weight:500;letter-spacing:-.01em;color:var(--accent);}"
      ".project-meta{margin:0;color:var(--muted);font-size:11px;letter-spacing:.06em;text-transform:uppercase;}"
      ".project-lines{display:grid;gap:10px;margin-top:18px;}"
      ".project-record{display:grid;grid-template-columns:86px minmax(0,1fr);gap:10px;padding-top:10px;border-top:1px solid var(--line);margin:0;}"
      ".project-label{font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".project-copy{color:#d4d4d4;font-size:.91rem;}"
      ".project-action{display:inline-flex;align-items:center;gap:10px;width:max-content;margin-top:22px;padding:8px 0 0;background:none;border:0;border-top:1px solid var(--line-strong);color:var(--fg);text-decoration:none;font-size:11px;letter-spacing:.08em;text-transform:uppercase;cursor:pointer;}"
      ".project-action:hover{color:var(--accent);border-top-color:var(--accent);}"
      ".project-action-static{color:var(--muted);border-top-color:var(--line);cursor:default;}");

  used = appendf(
      out, out_cap, used,
      ".project-action:focus-visible,.dossier-close:focus-visible,.dossier-trigger:focus-visible{outline:1px solid var(--accent);outline-offset:4px;}"
      ".dossier-layer{position:fixed;inset:0;z-index:30;display:block;pointer-events:none;}"
      ".dossier-layer[hidden]{display:none}"
      ".dossier-layer.is-live{pointer-events:auto}"
      ".dossier-backdrop{position:absolute;inset:0;background:rgba(0,0,0,.74);opacity:0;transition:opacity .18s linear;}"
      ".dossier-shell{position:fixed;left:24px;top:48px;width:min(960px,calc(100vw - 48px));height:min(680px,calc(100vh - 96px));background:rgba(4,4,4,.985);border:1px solid var(--line-strong);box-shadow:0 32px 120px rgba(0,0,0,.5);overflow:hidden;opacity:0;transform-origin:top left;will-change:transform,opacity;}"
      ".dossier-shell::before{content:'';position:absolute;left:0;right:0;top:0;height:1px;background:linear-gradient(90deg,var(--accent),transparent 68%%);opacity:.35;}"
      ".dossier-shell::after{content:'';position:absolute;inset:1px;background:linear-gradient(180deg,rgba(255,255,255,.08),rgba(255,255,255,.025) 20%%,rgba(4,4,4,.96) 72%%);opacity:0;transform:translateY(0);pointer-events:none;transition:transform .2s cubic-bezier(0.32,1,0.68,1),opacity .14s linear;}"
      ".dossier-layer.is-open .dossier-backdrop{opacity:1}"
      ".dossier-layer.is-open .dossier-shell::after{opacity:.96;}"
      ".dossier-content{position:relative;height:100%%;overflow:auto;overscroll-behavior:contain;-webkit-overflow-scrolling:touch;padding:20px 20px 26px;opacity:0;transform:translateY(8px);transition:opacity .16s linear,transform .16s ease;}"
      ".dossier-layer.is-settled .dossier-shell::after{opacity:0;transform:translateY(-14%%);}"
      ".dossier-layer.is-settled .dossier-content{opacity:1;transform:none}"
      ".dossier-head{display:flex;justify-content:space-between;align-items:center;gap:16px;padding-bottom:14px;border-bottom:1px solid var(--line);}"
      ".dossier-label{margin:0;font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".dossier-close{appearance:none;background:none;border:0;padding:0;color:var(--muted);font-size:11px;letter-spacing:.08em;text-transform:uppercase;cursor:pointer;}"
      ".dossier-stamp{margin:14px 0 0;color:var(--muted);font-size:11px;letter-spacing:.08em;text-transform:uppercase;}"
      ".dossier-title{margin:10px 0 0;max-width:12ch;font-family:var(--font-display);font-size:clamp(2.1rem,4.8vw,3.7rem);line-height:.96;font-weight:500;letter-spacing:-.03em;color:var(--accent);}"
      ".dossier-lede{margin:18px 0 0;max-width:60ch;color:#d5d5d5;font-size:.94rem;line-height:1.68;}");

  used = appendf(
      out, out_cap, used,
      ".dossier-body{display:grid;grid-template-columns:minmax(240px,.78fr) minmax(0,1.22fr);gap:32px;margin-top:26px;align-items:start;}"
      ".dossier-section{padding-top:14px;border-top:1px solid var(--line);}"
      ".dossier-rail{display:grid;gap:18px;align-content:start;align-self:start;position:sticky;top:0;}"
      ".dossier-contents{margin:10px 0 0;padding:0;list-style:none;display:grid;gap:8px;}"
      ".dossier-contents-item{margin:0;}"
      ".dossier-toc-link{display:grid;grid-template-columns:38px minmax(0,1fr);gap:12px;align-items:start;color:#cfcfcf;text-decoration:none;font-size:.9rem;transition:color .22s ease;}"
      ".dossier-toc-index{font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".dossier-toc-text{display:block;}"
      ".dossier-toc-link:hover,.dossier-toc-link.is-selected{color:var(--accent);}"
      ".dossier-paper{display:grid;gap:18px;align-content:start;}"
      ".dossier-paper-section{padding-top:14px;position:relative;scroll-margin-top:18px;}"
      ".dossier-paper-section.is-targeted::before{content:'';position:absolute;left:-12px;right:-12px;top:6px;bottom:-10px;border:1px solid rgba(255,255,255,.34);opacity:0;pointer-events:none;animation:targetCue 1.24s cubic-bezier(0.22,0.61,0.36,1) forwards;}"
      ".dossier-paper-head{display:grid;grid-template-columns:44px minmax(0,1fr);gap:14px;align-items:start;}"
      ".dossier-step-index{font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".dossier-step-label{margin:0;font-family:var(--font-display);font-size:1.06rem;line-height:1.02;letter-spacing:-.01em;color:var(--accent);}"
      ".dossier-copy{margin:6px 0 0;max-width:58ch;color:#d4d4d4;font-size:.92rem;}"
      ".dossier-metrics{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px;margin-top:10px;}"
      ".dossier-metric{padding-top:12px;border-top:1px solid var(--line);}"
      ".dossier-metric-value{display:block;font-family:var(--font-display);font-size:1.68rem;line-height:.95;letter-spacing:-.02em;color:var(--accent);}"
      ".dossier-metric-label{display:block;margin-top:6px;color:var(--muted);font-size:11px;letter-spacing:.08em;text-transform:uppercase;}"
      ".dossier-note{max-width:38ch;color:#c8c8c8;}"
      ".footer-group{display:flex;align-items:center;gap:18px;flex-wrap:wrap;}"
      ".footer-link{appearance:none;background:none;border:0;padding:0;color:var(--muted);font:inherit;letter-spacing:.08em;text-transform:uppercase;cursor:pointer;}"
      ".footer-link:hover{color:var(--accent);}"
      "footer{display:flex;justify-content:space-between;align-items:center;gap:16px;flex-wrap:wrap;margin-top:34px;padding-top:14px;border-top:1px solid var(--line);color:var(--muted);font-size:11px;}"
      "::selection{background:var(--accent);color:#020202;}"
      "@keyframes rise{from{opacity:0;transform:translateY(4px)}to{opacity:1;transform:translateY(0)}}"
      "@keyframes targetCue{0%%{opacity:0}16%%{opacity:0}32%%{opacity:.82}54%%{opacity:.16}72%%{opacity:.52}100%%{opacity:0}}"
      "@media (max-width:860px){.systems-bar,.projects,.dossier-body,.dossier-metrics{grid-template-columns:1fr;}.project-card,.project-card:first-child{grid-column:span 1;min-height:auto;}.project-record{grid-template-columns:1fr;gap:6px;}h1,.dossier-title{max-width:none;}.dossier-shell{left:8px;top:8px;width:calc(100vw - 16px);height:calc(100vh - 16px);}.dossier-content{padding:16px 16px 20px;}.dossier-rail{position:static;}}"
      "@media (prefers-reduced-motion:reduce){html{scroll-behavior:auto}.site-main,.project-card,.dossier-backdrop,.dossier-content,.dossier-shell,.dossier-toc-link,.dossier-paper-section.is-targeted::before{transition:none;animation:none}}"
      "</style>"
      "</head>"
      "<body>"
      "<main class=\"site-main\">"
      "<header class=\"meta\">"
      "<span>Utkarsh Ambati</span>"
      "</header>"
      "<section class=\"hero\">"
      "<p class=\"kicker\">Constraint. Measurement. Tradeoff.</p>"
      "<h1>Built from first principles. Measured before described.</h1>"
      "<div class=\"systems-bar\">");

  for (size_t i = 0; i < SYSTEM_FACT_COUNT; ++i) {
    used = append_system_fact(out, out_cap, used, &SYSTEM_FACTS[i]);
  }

  used = appendf(
      out, out_cap, used,
      "</div>"
      "</section>"
      "<section>"
      "<div class=\"section-head\">"
      "<h2>Selected systems</h2>"
      "<div class=\"footnote\">Compressed notes</div>"
      "</div>"
      "<div class=\"projects\">");

  for (size_t i = 0; i < PROJECT_COUNT; ++i) {
    used = append_project_card(out, out_cap, used, &PROJECTS[i], i);
  }

  used = appendf(out, out_cap, used, "</div>");

  used = appendf(
      out, out_cap, used,
      "</section>"
      "<footer>"
      "<span>Static export from raw C</span>"
      "<div class=\"footer-group\">"
      "<button class=\"footer-link dossier-trigger\" type=\"button\" "
      "aria-haspopup=\"dialog\" aria-controls=\"site-dossier\" aria-expanded=\"false\">Implementation Notes</button>"
      "<span>Updated %s %s</span>"
      "</div>"
      "</footer>"
      "</main>",
      __DATE__, __TIME__);

  for (size_t i = 0; i < DOSSIER_COUNT; ++i) {
    used = append_dossier_overlay(out, out_cap, used, DOSSIERS[i].prefix,
                                  DOSSIERS[i].dossier);
  }
  used = append_dossier_script(out, out_cap, used);
  used = appendf(out, out_cap, used, "</body></html>");

  if (used >= out_cap) {
    out[out_cap - 1] = '\0';
    return out_cap - 1;
  }
  return used;
}
