document.addEventListener("DOMContentLoaded", function () {
  mermaid.initialize({ startOnLoad: false });

  const blocks = document.querySelectorAll(".highlight-mermaid pre");

  blocks.forEach((block) => {
    const code = block.textContent;
    const container = block.closest(".highlight-mermaid");

    if (!container) return;

    // 새 div 생성
    const mermaidDiv = document.createElement("div");
    mermaidDiv.className = "mermaid";
    mermaidDiv.textContent = code;

    // 교체
    container.parentNode.replaceChild(mermaidDiv, container);
  });

  // 실행
  mermaid.init(undefined, document.querySelectorAll(".mermaid"));
});
