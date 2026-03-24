#include "page.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct project {
  const char *name;
  const char *stack;
  const char *summary;
  const char *repo;
};

static const struct project PROJECTS[] = {
    {"Intraday Session Reopen Study",
     "Python, futures research, walk-forward evaluation",
     "ES futures reopen study built around session labeling, normalized "
     "state features, and transaction-cost stress tests.",
     ""},
    {"Blockchain Consensus Simulator", "Python, SHA-256, sockets, Docker",
     "Multi-node platform for proof-of-work validation, peer "
     "synchronization, and repeatable distributed testing.",
     ""},
    {"Financial Return Forecasting", "Python, TensorFlow, GRU/LSTM",
     "Rolling-window forecasting pipelines benchmarked against simpler "
     "baselines to test stability across regimes.",
     ""},
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
      "<meta name=\"description\" content=\"Selected work by Utkarsh Ambati.\">"
      "<title>Utkarsh Ambati</title>"
      "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">"
      "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>"
      "<link href=\"https://fonts.googleapis.com/css2?family=Bodoni+Moda:opsz,wght@6..96,400;6..96,500;6..96,600&family=IBM+Plex+Mono:wght@400;500&display=swap\" rel=\"stylesheet\">"
      "<style>"
      ":root{--bg:#000000;--panel:#050505;--fg:#f5f5f5;--muted:#8c8c8c;"
      "--accent:#ffffff;--line:rgba(255,255,255,.12);--line-strong:rgba(255,255,255,.3);"
      "--font-display:'Bodoni Moda',Didot,'Times New Roman',serif;"
      "--font-text:'IBM Plex Mono',ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,"
      "'Liberation Mono','Courier New',monospace;}"
      "*{box-sizing:border-box}"
      "html{scroll-behavior:smooth}"
      "body{margin:0;padding:0;background:"
      "radial-gradient(circle at 50%% -10%%,rgba(255,255,255,.07),transparent 28%%),"
      "linear-gradient(180deg,#020202 0%%,var(--bg) 100%%);color:var(--fg);"
      "font-family:var(--font-text);font-size:14px;line-height:1.72;"
      "letter-spacing:.01em;}"
      "body::before{content:\"\";position:fixed;inset:0;pointer-events:none;"
      "background-image:repeating-linear-gradient(180deg,rgba(255,255,255,.018) 0 1px,transparent 1px 4px);"
      "mask-image:linear-gradient(180deg,rgba(0,0,0,.5),transparent 76%%);}"
      "main{max-width:1020px;margin:0 auto;padding:44px 22px 88px;position:relative;}"
      ".meta,.project-meta,.footnote,.project-index,.kicker,.repo{font-family:var(--font-text);}"
      ".meta{display:flex;justify-content:space-between;align-items:center;gap:14px;"
      "font-size:11px;letter-spacing:.08em;text-transform:uppercase;color:var(--muted);"
      "padding-bottom:16px;border-bottom:1px solid var(--line);}"
      ".hero{display:grid;grid-template-columns:minmax(0,1.45fr) minmax(260px,1fr);"
      "gap:30px;align-items:end;padding:46px 0 26px;}"
      "h1{margin:0;font-family:var(--font-display);font-size:clamp(3rem,6.4vw,5rem);"
      "line-height:.92;font-weight:500;letter-spacing:-.03em;max-width:9ch;color:var(--accent);}"
      ".lede{margin:0;max-width:34ch;font-size:.98rem;color:var(--fg);}"
      ".lede strong{font-weight:600;color:var(--accent);}"
      ".kicker{margin:0 0 10px;font-size:11px;letter-spacing:.08em;text-transform:uppercase;"
      "color:var(--muted);}"
      ".principles{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px;"
      "margin:0;padding:0;list-style:none;}"
      ".principles li{padding-top:12px;border-top:1px solid var(--line);color:#c9c9c9;font-size:.92rem;}");

  used = appendf(
      out, out_cap, used,
      ".section-head{display:flex;justify-content:space-between;align-items:end;gap:16px;"
      "margin-top:30px;padding-bottom:12px;border-bottom:1px solid var(--line);}"
      ".section-head h2{margin:0;font-family:var(--font-display);font-size:1.02rem;"
      "font-weight:500;letter-spacing:0;color:var(--fg);}"
      ".footnote{font-size:11px;color:var(--muted);text-transform:uppercase;letter-spacing:.08em;}"
      ".projects{display:grid;grid-template-columns:repeat(12,minmax(0,1fr));gap:14px;margin-top:18px;}"
      "article{grid-column:span 6;padding:16px 16px 15px;background:var(--panel);"
      "border:1px solid var(--line);min-height:214px;display:flex;"
      "flex-direction:column;justify-content:space-between;position:relative;overflow:hidden;"
      "animation:rise .3s ease both;}"
      "article:first-child{grid-column:span 12;min-height:238px;}"
      "article::before{content:\"\";position:absolute;left:0;right:0;top:0;height:1px;"
      "background:linear-gradient(90deg,var(--line-strong),transparent 65%%);}"
      "article::after{content:\"\";position:absolute;left:16px;right:16px;bottom:0;height:1px;"
      "background:linear-gradient(90deg,var(--line-strong),transparent);}"
      ".project-top{display:flex;justify-content:space-between;gap:16px;align-items:flex-start;}"
      ".project-index{font-size:11px;color:var(--muted);letter-spacing:.08em;text-transform:uppercase;}"
      ".project-title{margin:4px 0 0;font-family:var(--font-display);font-size:1.26rem;"
      "line-height:1.04;font-weight:500;letter-spacing:-.01em;color:var(--accent);}"
      ".project-meta{margin:0;color:var(--muted);font-size:11px;letter-spacing:.06em;text-transform:uppercase;}"
      ".project-summary{margin:20px 0 0;max-width:46ch;color:#cfcfcf;font-size:.93rem;}"
      ".repo{display:inline-flex;align-items:center;gap:10px;width:max-content;margin-top:26px;"
      "padding-top:8px;color:var(--fg);text-decoration:none;font-size:11px;letter-spacing:.08em;"
      "text-transform:uppercase;border-top:1px solid var(--line-strong);}"
      ".repo:hover{color:var(--accent);border-top-color:var(--accent);}"
      ".repo-static{color:var(--muted);border-top-color:var(--line);}");

  used = appendf(
      out, out_cap, used,
      ".note-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:14px;margin-top:18px;}"
      ".note{padding:14px 0;border-top:1px solid var(--line);}"
      ".note-title{margin:0 0 6px;font-size:11px;color:var(--muted);letter-spacing:.08em;text-transform:uppercase;}"
      ".note p{margin:0;color:#cfcfcf;font-size:.92rem;}"
      "footer{display:flex;justify-content:space-between;align-items:center;gap:16px;flex-wrap:wrap;"
      "margin-top:34px;padding-top:14px;border-top:1px solid var(--line);color:var(--muted);font-size:11px;}"
      "::selection{background:var(--accent);color:#020202;}"
      "@keyframes rise{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:translateY(0)}}"
      "@media (max-width:860px){.hero,.projects,.note-grid,.principles{grid-template-columns:1fr;}"
      "article,article:first-child{grid-column:span 1;min-height:auto;}h1{max-width:none;}"
      ".project-summary{max-width:none;}}"
      "@media (prefers-reduced-motion:reduce){html{scroll-behavior:auto}article{animation:none}}"
      "</style>"
      "</head>"
      "<body>"
      "<main>"
      "<header class=\"meta\">"
      "<span>Utkarsh Ambati</span>"
      "<span>Portfolio</span>"
      "</header>"
      "<section class=\"hero\">"
      "<div>"
      "<p class=\"kicker\">Selected Work</p>"
      "<h1>Quiet surfaces. Exact internals.</h1>"
      "</div>");

  used = appendf(
      out, out_cap, used,
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
    const struct project *p = &PROJECTS[i];
    used = appendf(
        out, out_cap, used,
        "<article>"
        "<div class=\"project-top\">"
        "<div>"
        "<div class=\"project-index\">%02zu</div>"
        "<h3 class=\"project-title\">%s</h3>"
        "</div>"
        "<p class=\"project-meta\">%s</p>"
        "</div>"
        "<p class=\"project-summary\">%s</p>",
        i + 1, p->name, p->stack, p->summary);

    if (p->repo != NULL && p->repo[0] != '\0') {
      used = appendf(
          out, out_cap, used,
          "<a class=\"repo\" href=\"%s\" rel=\"noopener noreferrer\">Repository</a>",
          p->repo);
    } else {
      used = appendf(out, out_cap, used,
                     "<span class=\"repo repo-static\">Private</span>");
    }

    used = appendf(out, out_cap, used, "</article>");
  }

  used = appendf(
      out, out_cap, used,
      "</div>"
      "</section>"
      "<footer>"
      "<span>Utkarsh Ambati</span>"
      "<span>Updated %s %s</span>"
      "</footer>"
      "</main>"
      "</body>"
      "</html>",
      __DATE__, __TIME__);

  if (used >= out_cap) {
    out[out_cap - 1] = '\0';
    return out_cap - 1;
  }
  return used;
}
