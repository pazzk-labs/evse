# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information
import subprocess
import re
import unicodedata
from docutils import nodes
from sphinx.transforms import SphinxTransform

subprocess.call('cd ../ ; doxygen Doxyfile', shell=True)

project = 'Pazzk'
copyright = '2025, Pazzk'
author = ''

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'breathe',
    'sphinxcontrib.plantuml',
    'sphinx.ext.intersphinx',
    'myst_parser',
]
source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

templates_path = ['_templates']
exclude_patterns = []

breathe_default_project = "pazzk"
breathe_projects = {
    "pazzk": "../build/doxygen/xml"
}
breathe_show_define_initializer = True
breathe_show_enumvalue_initializer = True

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_book_theme'
html_static_path = ['_static']
html_logo = '_static/pazzk-logo.png'
html_extra_path = ['_extra_static']
html_js_files = [
    'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js',
    'mermaid-init.js',
]


def slugify(text):
    text = unicodedata.normalize('NFC', text)
    text = ''.join(c for c in text if not unicodedata.combining(c))
    text = text.lower()
    text = re.sub(r'[^\w가-힣\s-]', '', text)
    text = re.sub(r'\s+', '-', text)
    return text.strip('-')


class ForceSlugAnchorTransform(SphinxTransform):
    default_priority = 1  # 매우 이르게 실행

    def apply(self):
        for section in self.document.traverse(nodes.section):
            title_node = section.next_node(nodes.title)
            if not title_node:
                continue

            # 슬러그 생성
            slug = slugify(title_node.astext())
            if not slug:
                continue

            # 기존 ID 제거 후 슬러그로 덮어쓰기
            section['ids'] = [slug]


class GithubAdmonitionTransform(SphinxTransform):
    default_priority = 700

    def apply(self):
        for node in self.document.traverse(nodes.block_quote):
            if not node.children:
                continue

            first = node.children[0]
            if not isinstance(first, nodes.paragraph):
                continue

            text = first.astext()
            if text.startswith("[!") and "]" in text:
                # 예: [!NOTE]
                kind = text[2:].split("]")[0].strip().lower()
                title = kind.capitalize()

                # 인라인 요소 중 [!NOTE] 제거
                body_inlines = first.children
                # 첫 번째 strong/inline 텍스트 제거
                if body_inlines and body_inlines[0].astext().startswith(f"[!{kind.upper()}]"):
                    body_inlines = body_inlines[1:]

                # 나머지 본문 노드 구성
                new_body = nodes.paragraph('', '', *body_inlines)

                # 새 admonition 노드 생성
                admonition_node = nodes.admonition('', classes=[kind])
                admonition_node += nodes.title('', title)
                admonition_node += new_body

                # 기존 block_quote 교체
                node.replace_self(admonition_node)


def setup(app):
    app.add_transform(ForceSlugAnchorTransform)
    app.add_transform(GithubAdmonitionTransform)
