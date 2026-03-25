#include "page.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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
  const char *summary;
  const char *repo;
  bool interactive;
  const char *cta_label;
  const struct dossier *dossier;
};

static const struct dossier ES_DOSSIER = {
    "ES Microedge Study",
    "CME ES, Jun 2010-Apr 2025",
    "Objective: test whether session segmentation and a low-range state variable explain a stable intraday mean-reversion pattern in ES, then report the result inside explicit proxy assumptions.",
    {
        {"01", "Research question",
         "Conditional on a compressed Asia range, do one-minute ES shocks mean-revert once the reopen disturbance has cleared?"},
        {"02", "Sample",
         "5,194,387 one-minute ES bars from June 6, 2010 through April 25, 2025, labeled on an 18:00 ET trading day."},
        {"03", "Series construction",
         "For each minute t, select the highest-volume outright contract. Exclude spreads and synthetics, then stitch the outright into a continuous proxy series."},
        {"04", "State variable",
         "Define consolidation from Asia-session range scaled by realized variation. The filter retains 1,528 of 3,820 days with an Asia session."},
        {"05", "Reopen diagnostic",
         "Mean absolute movement in the first 10 reopen minutes is 28.45 ticks versus 2.23 ticks in minutes 20-30, a 12.76x shock ratio."},
        {"06", "Europe-open tail",
         "With a 23-tick absolute-return threshold, the Europe-open tail rate is 1.23x the rate observed outside the Europe-open window."},
        {"07", "Candidate rule",
         "On consolidation days only, fade +/-2 tick one-minute shocks during Asia; skip the first 10 reopen minutes; hold for 5 minutes."},
        {"08", "Proxy result",
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

static const struct project PROJECTS[] = {
    {"ES Microedge Study",
     "Python, futures data research, session modeling, backtesting",
     "Session-conditioned ES futures research note on reopen shock, Europe-open tail behavior, and a consolidation-gated Asia mean-reversion rule.",
     "",
     true,
     "Open Study",
     &ES_DOSSIER},
    {"Blockchain Consensus Simulator",
     "Python, SHA-256, sockets, Docker",
     "Multi-node platform for proof-of-work validation, peer synchronization, and repeatable distributed testing.",
     "",
     false,
     NULL,
     NULL},
    {"Financial Return Forecasting",
     "Python, TensorFlow, GRU/LSTM",
     "Rolling-window forecasting pipelines benchmarked against simpler baselines to test stability across regimes.",
     "",
     false,
     NULL,
     NULL},
};

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

static const struct dossier *primary_dossier(void) {
  for (size_t i = 0; i < PROJECT_COUNT; ++i) {
    if (PROJECTS[i].interactive && PROJECTS[i].dossier != NULL) {
      return PROJECTS[i].dossier;
    }
  }
  return NULL;
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
      "<p class=\"project-summary\">%s</p>",
      card_class, index + 1, project->name, project->stack, project->summary);

  if (project->interactive && project->dossier != NULL && has_text(project->cta_label)) {
    used = appendf(
        out, cap, used,
        "<button class=\"repo dossier-trigger\" type=\"button\" aria-haspopup=\"dialog\" "
        "aria-controls=\"study-dossier\" aria-expanded=\"false\">%s</button>",
        project->cta_label);
  } else if (has_text(project->repo)) {
    used = appendf(
        out, cap, used,
        "<a class=\"repo\" href=\"%s\" rel=\"noopener noreferrer\">Repository</a>",
        project->repo);
  } else {
    used = appendf(out, cap, used,
                   "<span class=\"repo repo-static\">Private</span>");
  }

  used = appendf(out, cap, used, "</article>");
  return used;
}

static size_t append_dossier(
    char *out,
    size_t cap,
    size_t used,
    const struct dossier *dossier) {
  if (dossier == NULL) {
    return used;
  }

  used = appendf(
      out, cap, used,
      "<div class=\"dossier-layer\" id=\"study-dossier-layer\" hidden>"
      "<div class=\"dossier-backdrop\" data-dossier-close></div>"
      "<section class=\"dossier-shell\" id=\"study-dossier\" role=\"dialog\" aria-modal=\"true\" "
      "aria-labelledby=\"study-dossier-title\">"
      "<div class=\"dossier-content\">"
      "<div class=\"dossier-head\">"
      "<p class=\"dossier-label\">Technical note</p>"
      "<button class=\"dossier-close\" type=\"button\" data-dossier-close>Close</button>"
      "</div>"
      "<p class=\"dossier-stamp\">%s</p>"
      "<h2 class=\"dossier-title\" id=\"study-dossier-title\">%s</h2>"
      "<p class=\"dossier-lede\">%s</p>"
      "<div class=\"dossier-body\">"
      "<aside class=\"dossier-column dossier-rail\">"
      "<section class=\"dossier-section\">"
      "<p class=\"dossier-label\">Sections</p>"
      "<ol class=\"dossier-contents\">",
      dossier->stamp, dossier->title, dossier->lede);

  for (size_t i = 0; i < dossier->step_count; ++i) {
    const struct dossier_step *step = &dossier->steps[i];
    used = appendf(
        out, cap, used,
        "<li class=\"dossier-contents-item\">"
        "<a class=\"dossier-toc-link\" href=\"#study-step-%s\">"
        "<span class=\"dossier-toc-index\">%s</span>"
        "<span class=\"dossier-toc-text\">%s</span>"
        "</a>"
        "</li>",
        step->index, step->index, step->label);
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
        "<section class=\"dossier-section dossier-paper-section\" id=\"study-step-%s\">"
        "<div class=\"dossier-paper-head\">"
        "<div class=\"dossier-step-index\">%s</div>"
        "<p class=\"dossier-step-label\">%s</p>"
        "</div>"
        "<p class=\"dossier-copy\">%s</p>"
        "</section>",
        step->index, step->index, step->label, step->detail);
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
      "const layer=document.getElementById('study-dossier-layer');"
      "const shell=document.getElementById('study-dossier');"
      "const main=document.querySelector('.site-main');"
      "const content=layer?layer.querySelector('.dossier-content'):null;"
      "const closeButton=layer?layer.querySelector('.dossier-close'):null;"
      "const backdrop=layer?layer.querySelector('.dossier-backdrop'):null;"
      "const triggers=Array.from(document.querySelectorAll('.dossier-trigger'));"
      "const tocLinks=Array.from(layer?layer.querySelectorAll('.dossier-toc-link'):[]);"
      "const paperSections=Array.from(layer?layer.querySelectorAll('.dossier-paper-section'):[]);"
      "const reduced=window.matchMedia('(prefers-reduced-motion: reduce)');"
      "let lastTrigger=null;"
      "let settleTimer=0;"
      "let collapseTimer=0;"
      "let closeTimer=0;"
      "let focusTimer=0;"
      "if(!layer||!shell||!main||!content||!closeButton||!backdrop||!triggers.length){return;}");

  used = appendf(
      out, cap, used,
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
      "function applyTargetRect(){"
      "const rect=targetRect();"
      "shell.style.left=rect.left+'px';"
      "shell.style.top=rect.top+'px';"
      "shell.style.width=rect.width+'px';"
      "shell.style.height=rect.height+'px';"
      "return rect;"
      "}"
      "function motionFrom(origin,target){"
      "const ox=origin.left+origin.width/2;"
      "const oy=origin.top+origin.height/2;"
      "const tx=target.left+target.width/2;"
      "const ty=target.top+target.height/2;"
      "const dx=ox-tx;"
      "const dy=oy-ty;"
      "const sx=Math.max(origin.width/target.width,0.08);"
      "const sy=Math.max(origin.height/target.height,0.06);"
      "return 'translate('+dx+'px,'+dy+'px) scale('+sx+','+sy+')';"
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
      "}");

  used = appendf(
      out, cap, used,
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
      "if(lastTrigger){lastTrigger.focus();lastTrigger.setAttribute('aria-expanded','false');}"
      "}"
      "function openDossier(trigger){"
      "if(!layer.hidden){return;}"
      "lastTrigger=trigger;"
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
      "const origin=trigger.getBoundingClientRect();"
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
      "});");

  used = appendf(
      out, cap, used,
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
      "const origin=lastTrigger?lastTrigger.getBoundingClientRect():target;"
      "shell.style.transition='transform 230ms cubic-bezier(0.55,0,0.8,0.2),opacity 170ms linear';"
      "requestAnimationFrame(function(){"
      "shell.style.transform=motionFrom(origin,target);"
      "shell.style.opacity='0.94';"
      "});"
      "closeTimer=window.setTimeout(finishClose,240);"
      "},120);");

  used = appendf(
      out, cap, used,
      "}"
      "triggers.forEach(function(trigger){"
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
      "})();"
      "</script>");

  return used;
}

size_t render_portfolio_html(char *out, size_t out_cap) {
  if (out_cap == 0) {
    return 0;
  }

  const struct dossier *dossier = primary_dossier();
  size_t used = 0;

  used = appendf(
      out, out_cap, used,
      "<!doctype html>"
      "<html lang=\"en\">"
      "<head>"
      "<meta charset=\"utf-8\">"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "<meta name=\"description\" content=\"Selected work by Utkarsh Ambati.\">"
      "<title>Utkarsh Ambati</title>"
      "<link rel=\"icon\" type=\"image/png\" sizes=\"512x512\" href=\"flopper.png\">"
      "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">"
      "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>"
      "<link href=\"https://fonts.googleapis.com/css2?family=Bodoni+Moda:opsz,wght@6..96,400;6..96,500;6..96,600&family=IBM+Plex+Mono:wght@400;500&display=swap\" rel=\"stylesheet\">"
      "<style>"
      ":root{--bg:#000000;--panel:#050505;--fg:#f5f5f5;--muted:#8c8c8c;"
      "--accent:#ffffff;--line:rgba(255,255,255,.12);--line-strong:rgba(255,255,255,.3);"
      "--font-display:'Bodoni Moda',Didot,'Times New Roman',serif;"
      "--font-text:'IBM Plex Mono',ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,'Liberation Mono','Courier New',monospace;}"
      "*{box-sizing:border-box}"
      "html{scroll-behavior:smooth}"
      "body{margin:0;padding:0;background:radial-gradient(circle at 50%% -10%%,rgba(255,255,255,.07),transparent 28%%),linear-gradient(180deg,#020202 0%%,var(--bg) 100%%);color:var(--fg);font-family:var(--font-text);font-size:14px;line-height:1.72;letter-spacing:.01em;}"
      "body::before{content:'';position:fixed;inset:0;pointer-events:none;background-image:repeating-linear-gradient(180deg,rgba(255,255,255,.018) 0 1px,transparent 1px 4px);mask-image:linear-gradient(180deg,rgba(0,0,0,.5),transparent 76%%);}"
      "body.dossier-open{overflow:hidden}"
      ".site-main{max-width:1020px;margin:0 auto;padding:44px 22px 88px;position:relative;transition:opacity .18s linear;}"
      "body.dossier-open .site-main{opacity:.25}"
      ".meta,.project-meta,.footnote,.project-index,.kicker,.repo,.dossier-label,.dossier-close,.dossier-metric-label{font-family:var(--font-text);}"
      ".meta{display:flex;justify-content:space-between;align-items:center;gap:14px;font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);padding-bottom:16px;border-bottom:1px solid var(--line);}"
      ".hero{display:grid;grid-template-columns:minmax(0,1.45fr) minmax(260px,1fr);gap:30px;align-items:end;padding:46px 0 26px;}"
      "h1{margin:0;font-family:var(--font-display);font-size:clamp(3rem,6.4vw,5rem);line-height:.92;font-weight:500;letter-spacing:-.03em;max-width:9ch;color:var(--accent);}"
      ".kicker{margin:0 0 10px;font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".principles{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px;margin:0;padding:0;list-style:none;}"
      ".principles li{padding-top:12px;border-top:1px solid var(--line);color:#c9c9c9;font-size:.92rem;}"
      ".section-head{display:flex;justify-content:space-between;align-items:end;gap:16px;margin-top:30px;padding-bottom:12px;border-bottom:1px solid var(--line);}"
      ".section-head h2{margin:0;font-family:var(--font-display);font-size:1.02rem;font-weight:500;letter-spacing:0;color:var(--fg);}"
      ".footnote{font-size:11px;color:var(--muted);text-transform:uppercase;letter-spacing:.08em;}");

  used = appendf(
      out, out_cap, used,
      ".projects{display:grid;grid-template-columns:repeat(12,minmax(0,1fr));gap:14px;margin-top:18px;}"
      ".project-card{grid-column:span 6;padding:16px 16px 15px;background:var(--panel);border:1px solid var(--line);min-height:214px;display:flex;flex-direction:column;justify-content:space-between;position:relative;overflow:hidden;animation:rise .3s ease both;}"
      ".project-card:first-child{grid-column:span 12;min-height:238px;}"
      ".project-card::before{content:'';position:absolute;left:0;right:0;top:0;height:1px;background:linear-gradient(90deg,var(--line-strong),transparent 65%%);}"
      ".project-card::after{content:'';position:absolute;left:16px;right:16px;bottom:0;height:1px;background:linear-gradient(90deg,var(--line-strong),transparent);}"
      ".project-card-interactive:hover{border-color:var(--line-strong);}"
      ".project-top{display:flex;justify-content:space-between;gap:16px;align-items:flex-start;}"
      ".project-index{font-size:11px;color:var(--muted);letter-spacing:.08em;text-transform:uppercase;}"
      ".project-title{margin:4px 0 0;font-family:var(--font-display);font-size:1.26rem;line-height:1.04;font-weight:500;letter-spacing:-.01em;color:var(--accent);}"
      ".project-meta{margin:0;color:var(--muted);font-size:11px;letter-spacing:.06em;text-transform:uppercase;}"
      ".project-summary{margin:20px 0 0;max-width:50ch;color:#cfcfcf;font-size:.93rem;}"
      ".repo{display:inline-flex;align-items:center;gap:10px;width:max-content;margin-top:26px;padding:8px 0 0;background:none;border:0;border-top:1px solid var(--line-strong);color:var(--fg);text-decoration:none;font-size:11px;letter-spacing:.08em;text-transform:uppercase;cursor:pointer;}"
      ".repo:hover{color:var(--accent);border-top-color:var(--accent);}"
      ".repo-static{color:var(--muted);border-top-color:var(--line);cursor:default;}"
      ".repo:focus-visible,.dossier-close:focus-visible{outline:1px solid var(--accent);outline-offset:4px;}");

  used = appendf(
      out, out_cap, used,
      ".dossier-layer{position:fixed;inset:0;z-index:30;display:block;pointer-events:none;}"
      ".dossier-layer[hidden]{display:none}"
      ".dossier-layer.is-live{pointer-events:auto}"
      ".dossier-backdrop{position:absolute;inset:0;background:rgba(0,0,0,.74);opacity:0;transition:opacity .18s linear;}"
      ".dossier-shell{position:fixed;left:24px;top:48px;width:min(960px,calc(100vw - 48px));height:min(680px,calc(100vh - 96px));background:rgba(4,4,4,.985);border:1px solid var(--line-strong);box-shadow:0 32px 120px rgba(0,0,0,.5);overflow:hidden;opacity:0;transform-origin:center center;will-change:transform,opacity;}"
      ".dossier-shell::before{content:'';position:absolute;left:0;right:0;top:0;height:1px;background:linear-gradient(90deg,var(--accent),transparent 68%%);opacity:.35;}"
      ".dossier-shell::after{content:'';position:absolute;inset:1px;background:linear-gradient(180deg,rgba(255,255,255,.08),rgba(255,255,255,.025) 20%%,rgba(4,4,4,.96) 72%%);opacity:0;transform:translateY(0);pointer-events:none;transition:transform .2s cubic-bezier(0.32,1,0.68,1),opacity .14s linear;}"
      ".dossier-layer.is-open .dossier-backdrop{opacity:1}"
      ".dossier-layer.is-open .dossier-shell::after{opacity:.96;}"
      ".dossier-content{position:relative;height:100%%;overflow:auto;padding:20px 20px 26px;opacity:0;transform:translateY(8px);transition:opacity .16s linear,transform .16s ease;}"
      ".dossier-layer.is-settled .dossier-shell::after{opacity:0;transform:translateY(-14%%);}"
      ".dossier-layer.is-settled .dossier-content{opacity:1;transform:none}"
      ".dossier-head{display:flex;justify-content:space-between;align-items:center;gap:16px;padding-bottom:14px;border-bottom:1px solid var(--line);}"
      ".dossier-label{margin:0;font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".dossier-close{appearance:none;background:none;border:0;padding:0;color:var(--muted);font-size:11px;letter-spacing:.08em;text-transform:uppercase;cursor:pointer;}"
      ".dossier-stamp{margin:14px 0 0;color:var(--muted);font-size:11px;letter-spacing:.08em;text-transform:uppercase;}"
      ".dossier-title{margin:10px 0 0;font-family:var(--font-display);font-size:clamp(2.3rem,5vw,4.2rem);line-height:.95;font-weight:500;letter-spacing:-.03em;max-width:10ch;color:var(--accent);}"
      ".dossier-lede{margin:18px 0 0;max-width:52ch;color:#d5d5d5;font-size:.95rem;line-height:1.68;}");

  used = appendf(
      out, out_cap, used,
      ".dossier-body{display:grid;grid-template-columns:minmax(230px,.72fr) minmax(0,1.28fr);gap:32px;margin-top:26px;align-items:start;}"
      ".dossier-section{padding-top:14px;border-top:1px solid var(--line);}"
      ".dossier-rail{display:grid;gap:18px;align-content:start;align-self:start;position:sticky;top:0;}"
      ".dossier-contents{margin:10px 0 0;padding:0;list-style:none;display:grid;gap:8px;}"
      ".dossier-contents-item{margin:0;}"
      ".dossier-toc-link{display:grid;grid-template-columns:38px minmax(0,1fr);gap:12px;align-items:start;color:#cfcfcf;text-decoration:none;font-size:.9rem;}"
      ".dossier-toc-index{color:var(--muted);font-size:11px;letter-spacing:.08em;text-transform:uppercase;}"
      ".dossier-toc-text{display:block;}"
      ".dossier-toc-link{transition:color .22s ease;}"
      ".dossier-toc-link:hover,.dossier-toc-link.is-selected{color:var(--accent);}"
      ".dossier-paper{display:grid;gap:18px;align-content:start;}"
      ".dossier-paper-section{padding-top:14px;position:relative;scroll-margin-top:18px;}"
      ".dossier-paper-section.is-targeted::before{content:'';position:absolute;left:-12px;right:-12px;top:6px;bottom:-10px;border:1px solid rgba(255,255,255,.34);opacity:0;pointer-events:none;animation:targetCue 1.24s cubic-bezier(0.22,0.61,0.36,1) forwards;}"
      ".dossier-paper-head{display:grid;grid-template-columns:44px minmax(0,1fr);gap:14px;align-items:start;}"
      ".dossier-step-index{font-family:var(--font-text);font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);}"
      ".dossier-step-label{margin:0;font-family:var(--font-display);font-size:1.08rem;line-height:1.02;letter-spacing:-.01em;color:var(--accent);}"
      ".dossier-copy{margin:6px 0 0;max-width:56ch;color:#d4d4d4;font-size:.92rem;}");

  used = appendf(
      out, out_cap, used,
      ".dossier-metrics{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px;margin-top:10px;}"
      ".dossier-metric{padding-top:12px;border-top:1px solid var(--line);}"
      ".dossier-metric-value{display:block;font-family:var(--font-display);font-size:1.72rem;line-height:.95;color:var(--accent);letter-spacing:-.02em;}"
      ".dossier-metric-label{display:block;margin-top:6px;color:var(--muted);font-size:11px;letter-spacing:.08em;text-transform:uppercase;}"
      ".dossier-note{max-width:38ch;color:#c8c8c8;}"
      "footer{display:flex;justify-content:space-between;align-items:center;gap:16px;flex-wrap:wrap;margin-top:34px;padding-top:14px;border-top:1px solid var(--line);color:var(--muted);font-size:11px;}"
      "::selection{background:var(--accent);color:#020202;}"
      "@keyframes rise{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:translateY(0)}}"
      "@keyframes targetCue{0%%{opacity:0}16%%{opacity:0}32%%{opacity:.82}54%%{opacity:.16}72%%{opacity:.52}100%%{opacity:0}}"
      "@media (max-width:860px){.hero,.projects,.principles,.dossier-body,.dossier-metrics{grid-template-columns:1fr;}.project-card,.project-card:first-child{grid-column:span 1;min-height:auto;}h1,.dossier-title{max-width:none;}.project-summary{max-width:none;}.dossier-shell{left:8px;top:8px;width:calc(100vw - 16px);height:calc(100vh - 16px);}.dossier-content{padding:16px 16px 20px;}.dossier-rail{position:static;}}"
      "@media (prefers-reduced-motion:reduce){html{scroll-behavior:auto}.site-main,.dossier-backdrop,.dossier-content,.dossier-shell,.project-card,.dossier-paper-section.is-targeted::before{transition:none;animation:none}}"
      "</style>"
      "</head>"
      "<body>"
      "<main class=\"site-main\">"
      "<header class=\"meta\">"
      "<span>Utkarsh Ambati</span>"
      "<span>Portfolio</span>"
      "</header>"
      "<section class=\"hero\">"
      "<div>"
      "<p class=\"kicker\">Selected Work</p>"
      "<h1>Quiet surfaces. Exact internals.</h1>"
      "</div>"
      "<div>"
      "<ul class=\"principles\">"
      "<li>Austerity.</li>"
      "<li>Deliberation.</li>"
      "<li>Simplicity.</li>"
      "</ul>"
      "</div>"
      "</section>"
      "<section>"
      "<div class=\"section-head\">"
      "<h2>Projects</h2>"
      "<div class=\"footnote\">Recent</div>"
      "</div>"
      "<div class=\"projects\">");

  for (size_t i = 0; i < PROJECT_COUNT; ++i) {
    used = append_project_card(out, out_cap, used, &PROJECTS[i], i);
  }

  used = appendf(
      out, out_cap, used,
      "</div>"
      "</section>"
      "<footer>"
      "<span>Utkarsh Ambati</span>"
      "<span>Updated %s %s</span>"
      "</footer>"
      "</main>",
      __DATE__, __TIME__);

  used = append_dossier(out, out_cap, used, dossier);
  used = append_dossier_script(out, out_cap, used);
  used = appendf(out, out_cap, used, "</body></html>");

  if (used >= out_cap) {
    out[out_cap - 1] = '\0';
    return out_cap - 1;
  }
  return used;
}
