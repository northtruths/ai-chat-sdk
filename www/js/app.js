/**
 * AI 聊天助手 前端逻辑
 * 与 ChatServer (httplib) 提供的 REST + SSE 接口对接。
 * 接口约定见服务端 ChatServer.cpp。
 */
(function () {
    "use strict";

    // 前端与服务端同源部署（静态资源挂载在 ./www），故用相对路径即可。
    const API_BASE = "";

    // ============ 全局状态 ============
    const state = {
        sessions: [],           // 会话列表
        models: [],             // 可用模型
        currentSessionId: null, // 当前选中的会话
        selectedModel: null,    // 模态框中选中的模型系列名
        sending: false          // 是否正在流式接收，避免重复发送
    };

    // ============ DOM 引用 ============
    const el = {
        sessionList: document.getElementById("sessionList"),
        sessionCount: document.getElementById("sessionCount"),
        welcomeView: document.getElementById("welcomeView"),
        chatView: document.getElementById("chatView"),
        messages: document.getElementById("messages"),
        chatTitle: document.getElementById("chatTitle"),
        chatModelTag: document.getElementById("chatModelTag"),
        messageInput: document.getElementById("messageInput"),
        sendBtn: document.getElementById("sendBtn"),
        charCount: document.getElementById("charCount"),
        newChatBtnTop: document.getElementById("newChatBtnTop"),
        newChatBtnWelcome: document.getElementById("newChatBtnWelcome"),
        modelModal: document.getElementById("modelModal"),
        modelGrid: document.getElementById("modelGrid"),
        modelCancelBtn: document.getElementById("modelCancelBtn"),
        modelConfirmBtn: document.getElementById("modelConfirmBtn"),
        toast: document.getElementById("toast")
    };

    const MAX_CHARS = 2000;

    // ============ 工具函数 ============

    /** HTML 转义，防止 XSS 及标签注入 */
    function escapeHtml(str) {
        return String(str)
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#39;");
    }

    /** 时间戳（秒或毫秒）格式化为 本地时间 */
    function formatTime(ts) {
        if (!ts) return "";
        // 服务端时间可能是秒级，做一次兼容
        let ms = Number(ts);
        if (ms < 1e12) ms *= 1000;
        const d = new Date(ms);
        if (isNaN(d.getTime())) return "";
        const pad = (n) => String(n).padStart(2, "0");
        const now = new Date();
        const sameDay =
            d.getFullYear() === now.getFullYear() &&
            d.getMonth() === now.getMonth() &&
            d.getDate() === now.getDate();
        const hm = `${pad(d.getHours())}:${pad(d.getMinutes())}`;
        if (sameDay) return hm;
        return `${d.getMonth() + 1}/${d.getDate()} ${hm}`;
    }

    let toastTimer = null;
    function showToast(msg) {
        el.toast.textContent = msg;
        el.toast.classList.remove("hidden");
        clearTimeout(toastTimer);
        toastTimer = setTimeout(() => el.toast.classList.add("hidden"), 2200);
    }

    // ============ marked 配置（markdown + 代码高亮） ============
    marked.setOptions({
        breaks: true,
        gfm: true,
        highlight: function (code, lang) {
            try {
                if (lang && hljs.getLanguage(lang)) {
                    return hljs.highlight(code, { language: lang }).value;
                }
                return hljs.highlightAuto(code).value;
            } catch (e) {
                return escapeHtml(code);
            }
        }
    });

    /**
     * 渲染 markdown -> 安全 HTML，并为代码块包裹头部（语言标签 + 复制按钮）。
     */
    function renderMarkdown(text) {
        const rawHtml = marked.parse(text || "");
        const tpl = document.createElement("div");
        tpl.innerHTML = rawHtml;

        // 为每个 <pre><code> 包一层带头部的容器
        tpl.querySelectorAll("pre > code").forEach((codeEl) => {
            const pre = codeEl.parentElement;
            const langMatch = (codeEl.className || "").match(/language-([\w-]+)/);
            const lang = langMatch ? langMatch[1] : "text";

            const wrapper = document.createElement("div");
            wrapper.className = "code-block";

            const header = document.createElement("div");
            header.className = "code-block-header";
            header.innerHTML =
                `<span class="code-lang">${escapeHtml(lang)}</span>` +
                `<button class="copy-btn" type="button">` +
                `<svg viewBox="0 0 24 24" width="13" height="13" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">` +
                `<rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect>` +
                `<path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg>` +
                `<span>复制</span></button>`;

            pre.parentNode.insertBefore(wrapper, pre);
            wrapper.appendChild(header);
            wrapper.appendChild(pre);
        });

        return tpl.innerHTML;
    }

    /** 代码复制：事件委托绑定在 messages 容器上 */
    el.messages.addEventListener("click", function (e) {
        const btn = e.target.closest(".copy-btn");
        if (!btn) return;
        const codeEl = btn.closest(".code-block").querySelector("pre code");
        const text = codeEl ? codeEl.innerText : "";
        const done = () => {
            const label = btn.querySelector("span");
            const old = label.textContent;
            btn.classList.add("copied");
            label.textContent = "已复制";
            setTimeout(() => {
                btn.classList.remove("copied");
                label.textContent = old;
            }, 1500);
        };
        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(text).then(done).catch(() => fallbackCopy(text, done));
        } else {
            fallbackCopy(text, done);
        }
    });

    function fallbackCopy(text, cb) {
        const ta = document.createElement("textarea");
        ta.value = text;
        ta.style.position = "fixed";
        ta.style.opacity = "0";
        document.body.appendChild(ta);
        ta.select();
        try { document.execCommand("copy"); cb && cb(); }
        catch (e) { showToast("复制失败"); }
        document.body.removeChild(ta);
    }

    // ============ API 调用 ============

    async function apiGetSessions() {
        const res = await fetch(`${API_BASE}/api/sessions`);
        const json = await res.json();
        return json.success ? (json.data || []) : [];
    }

    async function apiGetModels() {
        const res = await fetch(`${API_BASE}/api/models`);
        const json = await res.json();
        return json.success ? (json.data || []) : [];
    }

    async function apiCreateSession(model) {
        const res = await fetch(`${API_BASE}/api/session`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ model })
        });
        return res.json();
    }

    async function apiDeleteSession(sessionId) {
        const res = await fetch(`${API_BASE}/api/session/${encodeURIComponent(sessionId)}`, {
            method: "DELETE"
        });
        return res.json();
    }

    async function apiGetHistory(sessionId) {
        const res = await fetch(`${API_BASE}/api/session/${encodeURIComponent(sessionId)}/history`);
        const json = await res.json();
        return json.success ? (json.data || []) : [];
    }

    // ============ 侧边栏：会话列表渲染 ============

    function renderSessionList() {
        el.sessionCount.textContent = `共 ${state.sessions.length} 个会话`;

        if (!state.sessions.length) {
            el.sessionList.innerHTML =
                `<div class="session-empty">暂无历史会话<br/>点击「新建对话」开始</div>`;
            return;
        }

        el.sessionList.innerHTML = "";
        state.sessions.forEach((s) => {
            const card = document.createElement("div");
            card.className = "session-card" + (s.id === state.currentSessionId ? " active" : "");
            card.dataset.id = s.id;

            const title = s.first_user_message && s.first_user_message.trim()
                ? s.first_user_message.trim()
                : "新的对话";

            card.innerHTML =
                `<div class="sc-title">${escapeHtml(title)}</div>` +
                `<div class="sc-meta">` +
                    `<span class="sc-model">${escapeHtml(s.model || "")}</span>` +
                    `<span>${formatTime(s.updated_at)}</span>` +
                `</div>` +
                `<button class="sc-more" type="button" title="更多操作" aria-label="更多操作">···</button>` +
                `<div class="session-menu hidden">` +
                    `<button class="session-menu-item danger" type="button">删除会话</button>` +
                `</div>`;

            // 点击卡片打开会话
            card.addEventListener("click", () => openSession(s.id));
            // 点击省略号打开会话操作菜单
            const moreButton = card.querySelector(".sc-more");
            const sessionMenu = card.querySelector(".session-menu");
            moreButton.addEventListener("click", (e) => {
                e.stopPropagation();
                document.querySelectorAll(".session-menu").forEach((menu) => {
                    if (menu !== sessionMenu) menu.classList.add("hidden");
                });
                sessionMenu.classList.toggle("hidden");
            });
            sessionMenu.querySelector(".session-menu-item").addEventListener("click", (e) => {
                e.stopPropagation();
                sessionMenu.classList.add("hidden");
                handleDeleteSession(s.id);
            });

            el.sessionList.appendChild(card);
        });
    }

    // ============ 会话操作 ============

    async function loadSessions() {
        try {
            state.sessions = await apiGetSessions();
            // 按更新时间降序
            state.sessions.sort((a, b) => (b.updated_at || 0) - (a.updated_at || 0));
            renderSessionList();
        } catch (e) {
            showToast("获取会话列表失败");
        }
    }

    async function openSession(sessionId) {
        if (state.sending) {
            showToast("正在接收消息，请稍候");
            return;
        }
        state.currentSessionId = sessionId;
        const session = state.sessions.find((s) => s.id === sessionId);

        showChatView();
        el.chatModelTag.textContent = session ? (session.model || "") : "";
        el.chatTitle.textContent =
            session && session.first_user_message ? session.first_user_message.trim() : "对话";
        renderSessionList();

        el.messages.innerHTML = "";
        try {
            const history = await apiGetHistory(sessionId);
            history.forEach((m) => {
                appendMessage(m.role, m.content, m.timestamp);
            });
            scrollToBottom();
        } catch (e) {
            showToast("获取历史消息失败");
        }
    }

    async function handleDeleteSession(sessionId) {
        try {
            const json = await apiDeleteSession(sessionId);
            if (!json.success) {
                showToast(json.message || "删除失败");
                return;
            }
            state.sessions = state.sessions.filter((s) => s.id !== sessionId);
            if (state.currentSessionId === sessionId) {
                state.currentSessionId = null;
                showWelcomeView();
            }
            renderSessionList();
            showToast("会话已删除");
        } catch (e) {
            showToast("删除失败");
        }
    }

    // ============ 视图切换 ============

    function showWelcomeView() {
        el.chatView.classList.add("hidden");
        el.welcomeView.classList.remove("hidden");
    }

    function showChatView() {
        el.welcomeView.classList.add("hidden");
        el.chatView.classList.remove("hidden");
    }

    // ============ 消息渲染 ============

    /**
     * 追加一条消息。返回 bubble 元素（供流式更新使用）。
     */
    function appendMessage(role, content, timestamp) {
        const row = document.createElement("div");
        row.className = "msg-row " + (role === "user" ? "user" : "assistant");

        const wrap = document.createElement("div");
        wrap.className = "msg-wrap";

        const bubble = document.createElement("div");
        bubble.className = "bubble";
        if (role === "user") {
            // 用户消息按纯文本展示（转义 + 保留换行）
            bubble.innerHTML = escapeHtml(content).replace(/\n/g, "<br/>");

            const actions = document.createElement("div");
            actions.className = "msg-actions";
            const resendButton = document.createElement("button");
            resendButton.className = "msg-resend";
            resendButton.type = "button";
            resendButton.title = "重发消息";
            resendButton.textContent = "重发";
            resendButton.addEventListener("click", (event) => {
                event.stopPropagation();
                sendMessage(content);
            });
            actions.appendChild(resendButton);
            wrap.appendChild(bubble);
            wrap.appendChild(actions);
        } else {
            bubble.innerHTML = renderMarkdown(content);
            wrap.appendChild(bubble);
        }

        const time = document.createElement("div");
        time.className = "msg-time";
        time.textContent = formatTime(timestamp || Date.now());

        wrap.appendChild(time);
        row.appendChild(wrap);
        el.messages.appendChild(row);
        return { row, bubble, time };
    }

    function scrollToBottom() {
        el.messages.scrollTop = el.messages.scrollHeight;
    }

    // ============ 发送消息 - 流式 ============

    async function sendMessage(messageToResend = null) {
        const text = messageToResend === null
            ? el.messageInput.value.trim()
            : messageToResend.trim();
        if (!text || state.sending || !state.currentSessionId) return;
        if (text.length > MAX_CHARS) {
            showToast(`消息不能超过 ${MAX_CHARS} 字`);
            return;
        }

        // 渲染用户消息
        appendMessage("user", text, Date.now());
        // 若该会话是首条消息，更新侧栏标题
        maybeUpdateSessionTitle(text);

        if (messageToResend === null) {
            el.messageInput.value = "";
            updateCharCount();
            autoResize();
        }
        scrollToBottom();

        // 预置一个 assistant 空气泡用于流式填充
        const aiMsg = appendMessage("assistant", "", Date.now());
        aiMsg.bubble.classList.add("typing-cursor");
        scrollToBottom();

        state.sending = true;
        setSending(true);

        let accumulated = "";
        try {
            await streamMessage(state.currentSessionId, text, (chunk) => {
                accumulated += chunk;
                aiMsg.bubble.innerHTML = renderMarkdown(accumulated);
                scrollToBottom();
            });
        } catch (e) {
            aiMsg.bubble.classList.remove("typing-cursor");
            if (!accumulated) {
                aiMsg.bubble.innerHTML =
                    `<span style="color:var(--danger)">消息发送失败，请重试</span>`;
            }
            showToast("发送失败");
        } finally {
            aiMsg.bubble.classList.remove("typing-cursor");
            aiMsg.time.textContent = formatTime(Date.now());
            state.sending = false;
            setSending(false);
            // 刷新侧栏时间/消息数
            loadSessions();
        }
    }

    /**
     * 读取 SSE 流。
     * 服务端每段格式：data: "<JSON 字符串>"\n\n，结束标记 data: [DONE]。
     * 注意：服务端对每个 chunk 用 Json::valueToQuotedString 序列化，
     * 所以 data 后面是一个带引号、转义过的 JSON 字符串，需要 JSON.parse 还原。
     */
    async function streamMessage(sessionId, message, onChunk) {
        const res = await fetch(`${API_BASE}/api/message/async`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ session_id: sessionId, message })
        });

        if (!res.ok || !res.body) {
            throw new Error("stream request failed: " + res.status);
        }

        const reader = res.body.getReader();
        const decoder = new TextDecoder("utf-8");
        let buffer = "";

        while (true) {
            const { value, done } = await reader.read();
            if (done) break;

            buffer += decoder.decode(value, { stream: true });

            // 以空行(\n\n)分割 SSE 事件；保留最后不完整的一段在 buffer
            let sepIndex;
            while ((sepIndex = buffer.indexOf("\n\n")) !== -1) {
                const rawEvent = buffer.slice(0, sepIndex);
                buffer = buffer.slice(sepIndex + 2);
                handleSSEEvent(rawEvent, onChunk);
                if (streamDone) return;
            }
        }
        // 处理可能残留（无结尾空行）的一段
        if (buffer.trim()) handleSSEEvent(buffer, onChunk);
    }

    // 标记 [DONE]，用于跳出读取循环
    let streamDone = false;

    function handleSSEEvent(rawEvent, onChunk) {
        // 一个事件可能包含多行 data:
        const lines = rawEvent.split("\n");
        for (const line of lines) {
            const trimmed = line.replace(/\r$/, "");
            if (!trimmed.startsWith("data:")) continue;

            let payload = trimmed.slice(5); // 去掉 "data:"
            if (payload.startsWith(" ")) payload = payload.slice(1);

            if (payload === "[DONE]") {
                streamDone = true;
                return;
            }
            if (payload === "") continue;

            // 服务端把 chunk 用 JSON 引号包裹并转义，这里解析还原真实文本
            let text;
            try {
                text = JSON.parse(payload);
            } catch (e) {
                // 兜底：无法解析则按原文处理
                text = payload;
            }
            if (typeof text === "string" && text.length) {
                onChunk(text);
            }
        }
    }

    // 每次开始一个新的流之前重置 done 标记
    function beginStreamReset() {
        streamDone = false;
    }

    // ============ 侧栏标题即时更新 ============

    function maybeUpdateSessionTitle(text) {
        const s = state.sessions.find((x) => x.id === state.currentSessionId);
        if (s && (!s.first_user_message || !s.first_user_message.trim())) {
            s.first_user_message = text;
            el.chatTitle.textContent = text;
            renderSessionList();
        }
    }

    // ============ 模型选择模态框 ============

    async function openModelModal() {
        state.selectedModel = null;
        el.modelConfirmBtn.disabled = true;
        el.modelGrid.innerHTML = `<div class="session-empty">加载模型中...</div>`;
        el.modelModal.classList.remove("hidden");

        try {
            if (!state.models.length) {
                state.models = await apiGetModels();
            }
            renderModelGrid();
        } catch (e) {
            el.modelGrid.innerHTML = `<div class="session-empty">获取模型失败</div>`;
        }
    }

    function renderModelGrid() {
        if (!state.models.length) {
            el.modelGrid.innerHTML = `<div class="session-empty">暂无可用模型</div>`;
            return;
        }
        el.modelGrid.innerHTML = "";
        state.models.forEach((m) => {
            const card = document.createElement("div");
            card.className = "model-card";
            card.dataset.name = m.provider || m.name;
            card.innerHTML =
                `<span class="model-radio"></span>` +
                `<div class="model-info">` +
                    `<div class="model-name">${escapeHtml(m.name || "")}</div>` +
                    `<div class="model-desc">${escapeHtml(m.desc || "暂无描述")}</div>` +
                `</div>`;
            card.addEventListener("click", () => {
                // 单选：清除其它选中
                el.modelGrid.querySelectorAll(".model-card").forEach((c) =>
                    c.classList.remove("selected"));
                card.classList.add("selected");
                // 后端按 provider（模型系列名）查找已注册的 Provider。
                state.selectedModel = m.provider || m.name;
                el.modelConfirmBtn.disabled = false;
            });
            el.modelGrid.appendChild(card);
        });
    }

    function closeModelModal() {
        el.modelModal.classList.add("hidden");
    }

    async function confirmCreateSession() {
        if (!state.selectedModel) return;
        el.modelConfirmBtn.disabled = true;
        try {
            const json = await apiCreateSession(state.selectedModel);
            if (!json.success || !json.data) {
                showToast(json.message || "创建会话失败");
                el.modelConfirmBtn.disabled = false;
                return;
            }
            closeModelModal();

            const newSession = {
                id: json.data.session_id,
                model: json.data.model || state.selectedModel,
                created_at: Math.floor(Date.now() / 1000),
                updated_at: Math.floor(Date.now() / 1000),
                message_count: 0,
                first_user_message: ""
            };
            state.sessions.unshift(newSession);
            state.currentSessionId = newSession.id;
            renderSessionList();

            // 打开空会话视图
            showChatView();
            el.chatModelTag.textContent = newSession.model;
            el.chatTitle.textContent = "新的对话";
            el.messages.innerHTML = "";
            el.messageInput.focus();
        } catch (e) {
            showToast("创建会话失败");
            el.modelConfirmBtn.disabled = false;
        }
    }

    // ============ 输入框交互 ============

    function updateCharCount() {
        const len = el.messageInput.value.length;
        el.charCount.textContent = len;
        el.charCount.parentElement.classList.toggle("over", len >= MAX_CHARS);
    }

    function autoResize() {
        el.messageInput.style.height = "auto";
        el.messageInput.style.height = Math.min(el.messageInput.scrollHeight, 160) + "px";
    }

    function setSending(sending) {
        el.sendBtn.disabled = sending;
        el.messageInput.disabled = false; // 允许继续打字，但发送受 state.sending 拦截
    }

    // ============ 事件绑定 ============

    function bindEvents() {
        el.newChatBtnTop.addEventListener("click", openModelModal);
        el.newChatBtnWelcome.addEventListener("click", openModelModal);
        el.modelCancelBtn.addEventListener("click", closeModelModal);
        el.modelConfirmBtn.addEventListener("click", confirmCreateSession);

        // 点击遮罩关闭模态框
        el.modelModal.addEventListener("click", (e) => {
            if (e.target === el.modelModal) closeModelModal();
        });

        // 点击页面其它位置关闭会话操作菜单
        document.addEventListener("click", () => {
            document.querySelectorAll(".session-menu").forEach((menu) =>
                menu.classList.add("hidden"));
        });

        el.sendBtn.addEventListener("click", () => {
            beginStreamReset();
            sendMessage();
        });

        el.messageInput.addEventListener("input", () => {
            updateCharCount();
            autoResize();
        });

        // Enter 发送，Shift+Enter 换行
        el.messageInput.addEventListener("keydown", (e) => {
            if (e.key === "Enter" && !e.shiftKey) {
                e.preventDefault();
                beginStreamReset();
                sendMessage();
            }
        });
    }

    // ============ 初始化 ============

    function init() {
        bindEvents();
        updateCharCount();
        loadSessions(); // 载入后自动获取会话列表并展示
        showWelcomeView();
    }

    document.addEventListener("DOMContentLoaded", init);
})();
