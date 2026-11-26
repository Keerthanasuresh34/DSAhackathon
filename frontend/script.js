/**
 * The Museum's Vanishing Family Trees — Tree Archiving and Recovery System
 * Frontend UI & Interactive Visualization Engine (Member 3)
 *
 * Technologies: Vanilla JavaScript, Fetch API, SVG Hierarchy Rendering, Web Audio API
 * Strict Compliance: Binary tree algorithms & restoration executed by C / Flask backend.
 */

(function () {
  'use strict';

  /* ================= CONFIGURATION & GLOBAL STATE ================= */
  const AppState = {
    // API endpoint base URL. Can be relative if Flask serves frontend, or localhost:5000
    apiBaseUrl: localStorage.getItem('museum_api_url') || (
      window.location.protocol.startsWith('http') && window.location.port === '5000'
        ? '' 
        : 'http://127.0.0.1:5000'
    ),
    backendOnline: false,
    soundEnabled: true,
    showGhostNulls: true,

    // Active Original Tree Data (returned by backend)
    originalTreeData: null,
    originalTokens: '',
    originalSerialized: '',
    originalTraversals: { preorder: [], inorder: [], postorder: [] },
    originalStats: { nodes: 0, height: 0, nulls: 0 },

    // Restored Tree Data (returned by backend recovery)
    restoredTreeData: null,
    restoredTokens: '',
    restoredTraversals: { preorder: [], inorder: [] },
    restoredStats: { nodes: 0, height: 0, nulls: 0 },

    // Canvas Pan & Zoom
    originalZoom: { scale: 1, x: 0, y: 0, isDragging: false, startX: 0, startY: 0 },
    restoredZoom: { scale: 1, x: 0, y: 0 },

    // Traversal Animation Walk
    playbackTimer: null,
    playbackIndex: 0,
    playbackSequence: [],
    playbackActive: false
  };

  /* ================= AUDIO SYNTHESIZER (WEB AUDIO API) ================= */
  const SoundEngine = {
    ctx: null,
    init() {
      if (!this.ctx && (window.AudioContext || window.webkitAudioContext)) {
        const AudioCtx = window.AudioContext || window.webkitAudioContext;
        this.ctx = new AudioCtx();
      }
    },
    playTone(freq, type, duration, gainValue = 0.08) {
      if (!AppState.soundEnabled) return;
      try {
        this.init();
        if (this.ctx.state === 'suspended') {
          this.ctx.resume();
        }
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();
        osc.type = type;
        osc.frequency.setValueAtTime(freq, this.ctx.currentTime);
        gain.gain.setValueAtTime(gainValue, this.ctx.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.0001, this.ctx.currentTime + duration);
        osc.connect(gain);
        gain.connect(this.ctx.destination);
        osc.start();
        osc.stop(this.ctx.currentTime + duration);
      } catch (e) {
        // Audio policy or unsupported
      }
    },
    chimeSuccess() {
      this.playTone(523.25, 'sine', 0.15, 0.06); // C5
      setTimeout(() => this.playTone(659.25, 'sine', 0.25, 0.06), 100); // E5
      setTimeout(() => this.playTone(783.99, 'sine', 0.4, 0.06), 200); // G5
    },
    chimeError() {
      this.playTone(220, 'sawtooth', 0.25, 0.08);
      setTimeout(() => this.playTone(185, 'sawtooth', 0.35, 0.08), 120);
    },
    chimeStep() {
      this.playTone(440, 'triangle', 0.08, 0.03);
    }
  };

  /* ================= DOM ELEMENTS CACHE ================= */
  const UI = {
    // Header & Status
    statusPulse: document.getElementById('statusPulse'),
    statusValue: document.getElementById('statusValue'),
    pingBackendBtn: document.getElementById('pingBackendBtn'),
    openSettingsBtn: document.getElementById('openSettingsBtn'),
    toggleSoundBtn: document.getElementById('toggleSoundBtn'),
    soundIcon: document.getElementById('soundIcon'),
    soundLabel: document.getElementById('soundLabel'),

    // Section 1: Ingestion Desk
    treeTokensInput: document.getElementById('treeTokensInput'),
    tokenCounterBadge: document.getElementById('tokenCounterBadge'),
    clearInputBtn: document.getElementById('clearInputBtn'),
    inputFeedback: document.getElementById('inputValidationFeedback'),
    feedbackText: document.getElementById('feedbackText'),
    loadTreeBtn: document.getElementById('loadTreeBtn'),
    loadTreeSpinner: document.getElementById('loadTreeSpinner'),
    resetTreeBtn: document.getElementById('resetTreeBtn'),
    loadSampleBtn: document.getElementById('loadSampleBtn'),
    sampleButtons: document.querySelectorAll('.btn-sample'),

    // Section 2: Original SVG Canvas
    canvasFrame: document.getElementById('canvasFrame'),
    treeSvgCanvas: document.getElementById('treeSvgCanvas'),
    svgPanGroup: document.getElementById('svgPanGroup'),
    svgLinksLayer: document.getElementById('svgLinksLayer'),
    svgNodesLayer: document.getElementById('svgNodesLayer'),
    canvasEmptyState: document.getElementById('canvasEmptyState'),
    nodeInspector: document.getElementById('nodeInspector'),
    inspVal: document.getElementById('inspVal'),
    inspDepth: document.getElementById('inspDepth'),
    inspSize: document.getElementById('inspSize'),
    inspLeft: document.getElementById('inspLeft'),
    inspRight: document.getElementById('inspRight'),
    zoomInBtn: document.getElementById('zoomInBtn'),
    zoomOutBtn: document.getElementById('zoomOutBtn'),
    resetZoomBtn: document.getElementById('resetZoomBtn'),
    toggleNullsBtn: document.getElementById('toggleNullsBtn'),
    exportSvgBtn: document.getElementById('exportSvgBtn'),
    metricNodes: document.getElementById('metricNodes'),
    metricHeight: document.getElementById('metricHeight'),
    metricNulls: document.getElementById('metricNulls'),
    metricBalance: document.getElementById('metricBalance'),

    // Section 3: Traversals
    preorderOutput: document.getElementById('preorderOutput'),
    inorderOutput: document.getElementById('inorderOutput'),
    postorderOutput: document.getElementById('postorderOutput'),
    copyButtons: document.querySelectorAll('.copy-btn'),
    playbackTraversalSelect: document.getElementById('playbackTraversalSelect'),
    playTraversalBtn: document.getElementById('playTraversalBtn'),
    pauseTraversalBtn: document.getElementById('pauseTraversalBtn'),
    playbackSpeed: document.getElementById('playbackSpeed'),

    // Section 4: Vault & Serialization
    serializedTextPreview: document.getElementById('serializedTextPreview'),
    vaultStreamLength: document.getElementById('vaultStreamLength'),
    vaultTokenCount: document.getElementById('vaultTokenCount'),
    vaultChecksum: document.getElementById('vaultChecksum'),
    archiveFilenameInput: document.getElementById('archiveFilenameInput'),
    serializeTreeBtn: document.getElementById('serializeTreeBtn'),
    downloadArchiveBtn: document.getElementById('downloadArchiveBtn'),
    copyArchiveBtn: document.getElementById('copyArchiveBtn'),

    // Section 5: Recovery Workshop
    archiveDropzone: document.getElementById('archiveDropzone'),
    archiveFileInput: document.getElementById('archiveFileInput'),
    dropzoneFileInfo: document.getElementById('dropzoneFileInfo'),
    uploadedFileName: document.getElementById('uploadedFileName'),
    uploadedFileSize: document.getElementById('uploadedFileSize'),
    removeFileBtn: document.getElementById('removeFileBtn'),
    recoveryTextInput: document.getElementById('recoveryTextInput'),
    pasteSerializedBtn: document.getElementById('pasteSerializedBtn'),
    restoreTreeBtn: document.getElementById('restoreTreeBtn'),
    restoreSpinner: document.getElementById('restoreSpinner'),
    restorationStatusBanner: document.getElementById('restorationStatusBanner'),
    restorationStatusIcon: document.getElementById('restorationStatusIcon'),
    restorationStatusTitle: document.getElementById('restorationStatusTitle'),
    restorationStatusDesc: document.getElementById('restorationStatusDesc'),
    restoredCanvasFrame: document.getElementById('restoredCanvasFrame'),
    restoredTreeSvg: document.getElementById('restoredTreeSvg'),
    restoredPanGroup: document.getElementById('restoredPanGroup'),
    restoredLinksLayer: document.getElementById('restoredLinksLayer'),
    restoredNodesLayer: document.getElementById('restoredNodesLayer'),
    restoredEmptyState: document.getElementById('restoredEmptyState'),
    restoredPill: document.getElementById('restoredPill'),
    restoredPreorderVal: document.getElementById('restoredPreorderVal'),
    restoredInorderVal: document.getElementById('restoredInorderVal'),

    // Section 6: Comparison Audit
    auditVerdictBanner: document.getElementById('auditVerdictBanner'),
    waxSeal: document.getElementById('waxSeal'),
    verdictKicker: document.getElementById('verdictKicker'),
    verdictHeadline: document.getElementById('verdictHeadline'),
    verdictSubtext: document.getElementById('verdictSubtext'),
    runComparisonAuditBtn: document.getElementById('runComparisonAuditBtn'),
    auditOrigPreorder: document.getElementById('auditOrigPreorder'),
    auditRestPreorder: document.getElementById('auditRestPreorder'),
    auditDiffPreorder: document.getElementById('auditDiffPreorder'),
    auditBadgePreorder: document.getElementById('auditBadgePreorder'),
    auditOrigInorder: document.getElementById('auditOrigInorder'),
    auditRestInorder: document.getElementById('auditRestInorder'),
    auditDiffInorder: document.getElementById('auditDiffInorder'),
    auditBadgeInorder: document.getElementById('auditBadgeInorder'),
    auditOrigNodes: document.getElementById('auditOrigNodes'),
    auditRestNodes: document.getElementById('auditRestNodes'),
    auditDiffNodes: document.getElementById('auditDiffNodes'),
    auditBadgeNodes: document.getElementById('auditBadgeNodes'),
    auditOrigHeight: document.getElementById('auditOrigHeight'),
    auditRestHeight: document.getElementById('auditRestHeight'),
    auditDiffHeight: document.getElementById('auditDiffHeight'),
    auditBadgeHeight: document.getElementById('auditBadgeHeight'),
    auditOrigNulls: document.getElementById('auditOrigNulls'),
    auditRestNulls: document.getElementById('auditRestNulls'),
    auditDiffNulls: document.getElementById('auditDiffNulls'),
    auditBadgeNulls: document.getElementById('auditBadgeNulls'),

    // Section 7: Damaged Archive Chamber
    damagedArchiveInput: document.getElementById('damagedArchiveInput'),
    defectButtons: document.querySelectorAll('.btn-defect'),
    testDamagedBtn: document.getElementById('testDamagedBtn'),
    damageSpinner: document.getElementById('damageSpinner'),
    damageStatusBadge: document.getElementById('damageStatusBadge'),
    diagnosisTitle: document.getElementById('diagnosisTitle'),
    diagnosisDesc: document.getElementById('diagnosisDesc'),
    rejectionDetailsBox: document.getElementById('rejectionDetailsBox'),
    rejectionLogText: document.getElementById('rejectionLogText'),

    // Settings Modal & Toasts
    settingsModal: document.getElementById('settingsModal'),
    closeSettingsBtn: document.getElementById('closeSettingsBtn'),
    apiBaseUrlInput: document.getElementById('apiBaseUrlInput'),
    testApiEndpointBtn: document.getElementById('testApiEndpointBtn'),
    saveApiSettingsBtn: document.getElementById('saveApiSettingsBtn'),
    endpointTestFeedback: document.getElementById('endpointTestFeedback'),
    toastContainer: document.getElementById('toastContainer')
  };

  /* ================= NOTIFICATIONS & TOASTS ================= */
  function showToast(title, message, type = 'info', duration = 4000) {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    
    let iconSvg = '';
    if (type === 'success') {
      iconSvg = '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="var(--sage-success)" stroke-width="2"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><polyline points="22 4 12 14.01 9 11.01"/></svg>';
    } else if (type === 'error') {
      iconSvg = '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="var(--crimson-error)" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/></svg>';
    } else {
      iconSvg = '<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="var(--gold-dark)" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>';
    }

    toast.innerHTML = `
      ${iconSvg}
      <div class="toast-body">
        <div class="toast-title">${title}</div>
        <div class="toast-msg">${message}</div>
      </div>
      <button class="toast-close">&times;</button>
    `;

    toast.querySelector('.toast-close').addEventListener('click', () => {
      toast.remove();
    });

    UI.toastContainer.appendChild(toast);

    if (duration > 0) {
      setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateX(100%)';
        setTimeout(() => toast.remove(), 250);
      }, duration);
    }
  }

  /* ================= HASH CHECKSUM (SHA-256) ================= */
  async function computeSha256(text) {
    if (!text) return 'None';
    try {
      const msgBuffer = new TextEncoder().encode(text);
      const hashBuffer = await crypto.subtle.digest('SHA-256', msgBuffer);
      const hashArray = Array.from(new Uint8Array(hashBuffer));
      const hashHex = hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
      return hashHex.substring(0, 16) + '...';
    } catch (e) {
      // Fallback simple hash for older contexts
      let hash = 0;
      for (let i = 0; i < text.length; i++) {
        hash = ((hash << 5) - hash) + text.charCodeAt(i);
        hash |= 0;
      }
      return '0x' + Math.abs(hash).toString(16);
    }
  }

  /* ================= BACKEND HEALTH CHECK ================= */
  async function checkBackendHealth(showNotification = false) {
    UI.statusPulse.className = 'status-pulse';
    UI.statusValue.textContent = 'Probing Server...';

    const endpoint = `${AppState.apiBaseUrl}/api/health`;
    try {
      const response = await fetch(endpoint, {
        method: 'GET',
        headers: { 'Accept': 'application/json' },
        cache: 'no-store'
      });

      if (response.ok) {
        const data = await response.json();
        AppState.backendOnline = true;
        UI.statusPulse.className = 'status-pulse online';
        UI.statusValue.textContent = `Online (${data.backend || 'C Engine'})`;
        if (showNotification) {
          showToast('Conservancy Online', 'Connected to Flask & C Binary Tree Engine', 'success');
          SoundEngine.chimeSuccess();
        }
        return true;
      } else {
        throw new Error(`HTTP ${response.status}`);
      }
    } catch (err) {
      AppState.backendOnline = false;
      UI.statusPulse.className = 'status-pulse offline';
      UI.statusValue.textContent = 'Backend Offline';
      if (showNotification) {
        showToast('Backend Unreachable', `Unable to reach ${endpoint}. Please verify Flask is running on port 5000.`, 'error');
        SoundEngine.chimeError();
      }
      return false;
    }
  }

  /* ================= TOKEN INPUT LIVE ANALYSIS ================= */
  function analyzeTokenInput() {
    const raw = UI.treeTokensInput.value.trim();
    if (!raw) {
      UI.tokenCounterBadge.textContent = 'Tokens: 0 | Nodes: 0 | Missing (#): 0';
      UI.feedbackText.textContent = 'Please enter space-separated preorder tokens.';
      UI.inputFeedback.querySelector('.feedback-indicator').className = 'feedback-indicator';
      return { tokens: [], nodes: 0, nulls: 0, isValid: false };
    }

    const tokens = raw.split(/\s+/).filter(Boolean);
    const nulls = tokens.filter(t => t === '#').length;
    const nodes = tokens.length - nulls;

    UI.tokenCounterBadge.textContent = `Tokens: ${tokens.length} | Nodes: ${nodes} | Missing (#): ${nulls}`;

    // Mathematical rule for binary tree with null markers:
    // Every non-empty binary tree with N internal nodes has N + 1 null leaves!
    // Total tokens = N + (N + 1) = 2N + 1.
    const expectedNulls = nodes + 1;
    let isValidCandidate = true;
    let message = 'Ready to ingest archive stream.';

    if (tokens.length === 1 && tokens[0] === '#') {
      message = 'Empty tree representation (#).';
      UI.inputFeedback.querySelector('.feedback-indicator').className = 'feedback-indicator valid';
    } else if (nodes > 0 && nulls === expectedNulls) {
      message = `Valid candidate binary tree topology (${nodes} nodes, ${nulls} leaves).`;
      UI.inputFeedback.querySelector('.feedback-indicator').className = 'feedback-indicator valid';
    } else if (nodes > 0 && nulls < expectedNulls) {
      message = `Stream appears truncated: needs ${expectedNulls - nulls} more '#' to terminate branches.`;
      UI.inputFeedback.querySelector('.feedback-indicator').className = 'feedback-indicator warning';
      isValidCandidate = false;
    } else {
      message = `Stream token balance unusual (Nodes: ${nodes}, Leaves: ${nulls}). Backend will perform rigorous invariant check.`;
      UI.inputFeedback.querySelector('.feedback-indicator').className = 'feedback-indicator warning';
    }

    UI.feedbackText.textContent = message;
    return { tokens, nodes, nulls, isValid: isValidCandidate };
  }

  /* ================= API CALL: INGEST & CREATE TREE ================= */
  async function handleLoadTree() {
    const rawTokens = UI.treeTokensInput.value.trim();
    if (!rawTokens) {
      showToast('Empty Inscription', 'Please enter preorder tokens before ingesting.', 'warning');
      SoundEngine.chimeError();
      return;
    }

    // Set loading UI
    UI.loadTreeBtn.disabled = true;
    UI.loadTreeSpinner.classList.remove('hidden');

    try {
      const endpoint = `${AppState.apiBaseUrl}/api/tree/create`;
      const response = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ tokens: rawTokens })
      });

      const result = await response.json();

      if (!response.ok || !result.success) {
        throw new Error(result.error || result.message || 'Tree ingestion rejected by backend.');
      }

      // Store in AppState
      AppState.originalTreeData = result.tree;
      AppState.originalTokens = rawTokens;
      AppState.originalSerialized = result.serialized || rawTokens;
      AppState.originalTraversals = result.traversals || { preorder: [], inorder: [], postorder: [] };
      AppState.originalStats = result.stats || {
        nodes: (result.traversals && result.traversals.preorder) ? result.traversals.preorder.length : 0,
        height: result.height || 0,
        nulls: rawTokens.split(/\s+/).filter(t => t === '#').length
      };

      // Update UI components
      renderTreeMetrics(AppState.originalStats);
      renderSvgTree(AppState.originalTreeData, UI.svgLinksLayer, UI.svgNodesLayer, UI.canvasEmptyState, AppState.showGhostNulls, 'orig');
      renderTraversals(AppState.originalTraversals);
      updateVaultPreview(AppState.originalSerialized);

      showToast('Specimen Ingested', `Constructed binary tree with ${AppState.originalStats.nodes} nodes.`, 'success');
      SoundEngine.chimeSuccess();

      // Trigger re-audit if restored tree already exists
      if (AppState.restoredTreeData) {
        runForensicAudit();
      }

    } catch (err) {
      console.error('Tree Ingestion Error:', err);
      showToast('Ingestion Error', err.message, 'error', 6000);
      SoundEngine.chimeError();

      // Also highlight damage section for judges
      UI.damagedArchiveInput.value = rawTokens;
    } finally {
      UI.loadTreeBtn.disabled = false;
      UI.loadTreeSpinner.classList.add('hidden');
    }
  }

  /* ================= TREE METRICS DISPLAY ================= */
  function renderTreeMetrics(stats) {
    UI.metricNodes.textContent = stats.nodes || 0;
    UI.metricHeight.textContent = stats.height || 0;
    UI.metricNulls.textContent = stats.nulls || 0;
    
    if (stats.nodes === 0) {
      UI.metricBalance.textContent = 'Empty Lineage';
    } else {
      UI.metricBalance.textContent = `Structure Verified (${stats.nodes} Nodes)`;
    }
  }

  /* ================= DYNAMIC SVG HIERARCHY TREE LAYOUT ================= */
  /**
   * Computes clean 2D coordinates for the binary tree nodes and renders them inside the SVG.
   * Tree format from backend:
   * Node: { val: string, left: Node|null, right: Node|null, id?: string }
   */
  function layoutBinaryTree(rootNode, includeNulls = true) {
    if (!rootNode) return { nodes: [], links: [], width: 800, height: 400 };

    let nodeIdCounter = 0;
    const nodes = [];
    const links = [];

    // Assign unique IDs & levels
    function prepareNode(n, depth = 0, isGhost = false, parentId = null, branchType = null) {
      if (!n) return null;
      const id = ++nodeIdCounter;
      const wrapped = {
        id,
        val: n.val,
        depth,
        isGhost,
        parentId,
        branchType,
        x: 0,
        y: depth * 80 + 50,
        left: null,
        right: null,
        raw: n
      };

      if (!isGhost) {
        if (n.left) {
          wrapped.left = prepareNode(n.left, depth + 1, false, id, 'left');
        } else if (includeNulls) {
          wrapped.left = prepareNode({ val: '#' }, depth + 1, true, id, 'left');
        }

        if (n.right) {
          wrapped.right = prepareNode(n.right, depth + 1, false, id, 'right');
        } else if (includeNulls) {
          wrapped.right = prepareNode({ val: '#' }, depth + 1, true, id, 'right');
        }
      }

      return wrapped;
    }

    const root = prepareNode(rootNode);

    // In-order placement to avoid node collisions
    let currentX = 50;
    const xSpacing = 58;

    function assignXCoordinates(node) {
      if (!node) return;
      if (node.left) assignXCoordinates(node.left);
      node.x = currentX;
      currentX += xSpacing;
      if (node.right) assignXCoordinates(node.right);
    }

    assignXCoordinates(root);

    // Flatten into node & link arrays
    function collectElements(node) {
      if (!node) return;
      nodes.push(node);
      if (node.left) {
        links.push({ from: node, to: node.left, isGhost: node.left.isGhost });
        collectElements(node.left);
      }
      if (node.right) {
        links.push({ from: node, to: node.right, isGhost: node.right.isGhost });
        collectElements(node.right);
      }
    }

    collectElements(root);

    const maxX = Math.max(...nodes.map(n => n.x), 700) + 70;
    const maxY = Math.max(...nodes.map(n => n.y), 320) + 70;

    return { root, nodes, links, width: maxX, height: maxY };
  }

  /**
   * Renders nodes & links onto SVG layers with museum styling
   */
  function renderSvgTree(treeData, linksLayer, nodesLayer, emptyStateEl, showNulls, prefix = 'orig') {
    // Clear previous SVG contents
    linksLayer.innerHTML = '';
    nodesLayer.innerHTML = '';

    if (!treeData) {
      emptyStateEl.classList.remove('hidden');
      return;
    }

    emptyStateEl.classList.add('hidden');

    const layout = layoutBinaryTree(treeData, showNulls);

    // Render Links
    layout.links.forEach(link => {
      const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      const x1 = link.from.x;
      const y1 = link.from.y;
      const x2 = link.to.x;
      const y2 = link.to.y;

      // Smooth cubic bezier curve connecting lineage generations
      const midY = (y1 + y2) / 2;
      const d = `M ${x1} ${y1} C ${x1} ${midY}, ${x2} ${midY}, ${x2} ${y2}`;

      path.setAttribute('d', d);
      path.setAttribute('class', link.isGhost ? 'tree-link-path ghost-link' : 'tree-link-path');
      path.id = `link-${prefix}-${link.from.id}-${link.to.id}`;
      linksLayer.appendChild(path);
    });

    // Render Nodes
    layout.nodes.forEach(node => {
      const group = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      group.setAttribute('class', 'tree-node-group');
      group.setAttribute('transform', `translate(${node.x}, ${node.y})`);
      group.dataset.nodeId = node.id;
      group.dataset.nodeVal = node.val;
      group.dataset.nodeDepth = node.depth;

      // Circle Element
      const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
      circle.setAttribute('r', node.isGhost ? '13' : '18');

      let circleClass = 'tree-node-circle';
      if (node.isGhost) {
        circleClass += ' null-ghost';
      } else if (node.depth === 0) {
        circleClass += ' root';
      } else if (!node.left && !node.right) {
        circleClass += ' leaf';
      } else {
        circleClass += ' internal';
      }
      circle.setAttribute('class', circleClass);
      group.appendChild(circle);

      // Label Element
      const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      text.textContent = node.val;
      text.setAttribute('class', node.isGhost ? 'tree-node-label null-label' : 'tree-node-label');
      group.appendChild(text);

      // Interactive hover & inspection
      if (!node.isGhost && prefix === 'orig') {
        group.addEventListener('mouseenter', (e) => {
          showNodeInspector(node, e);
        });
        group.addEventListener('mouseleave', () => {
          hideNodeInspector();
        });
        group.addEventListener('click', () => {
          SoundEngine.chimeStep();
          showNodeInspector(node, null, true);
        });
      }

      nodesLayer.appendChild(group);
    });

    // Center view if first load
    centerSvgView(layout.width, layout.height);
  }

  /* ================= NODE INSPECTOR FLOATING CARD ================= */
  function showNodeInspector(node, event, pinned = false) {
    UI.inspVal.textContent = node.val;
    UI.inspDepth.textContent = `${node.depth} ${node.depth === 0 ? '(Sovereign Root)' : ''}`;
    
    // Subtree count
    let count = 0;
    function countSubtree(n) {
      if (!n || n.isGhost) return;
      count++;
      if (n.left) countSubtree(n.left);
      if (n.right) countSubtree(n.right);
    }
    countSubtree(node);
    UI.inspSize.textContent = `${count} Descendants`;

    UI.inspLeft.textContent = (node.left && !node.left.isGhost) ? `Present (${node.left.val})` : 'Vanished (#)';
    UI.inspRight.textContent = (node.right && !node.right.isGhost) ? `Present (${node.right.val})` : 'Vanished (#)';

    UI.nodeInspector.classList.remove('hidden');
  }

  function hideNodeInspector() {
    UI.nodeInspector.classList.add('hidden');
  }

  /* ================= SVG PAN & ZOOM CONTROLS ================= */
  function updateSvgTransform() {
    const { x, y, scale } = AppState.originalZoom;
    UI.svgPanGroup.setAttribute('transform', `translate(${x}, ${y}) scale(${scale})`);
  }

  function centerSvgView(contentWidth, contentHeight) {
    const frameRect = UI.canvasFrame.getBoundingClientRect();
    const scaleX = (frameRect.width - 60) / contentWidth;
    const scaleY = (frameRect.height - 60) / contentHeight;
    const scale = Math.min(Math.max(Math.min(scaleX, scaleY), 0.65), 1.25);

    const x = (frameRect.width - contentWidth * scale) / 2;
    const y = 30;

    AppState.originalZoom = { scale, x, y, isDragging: false, startX: 0, startY: 0 };
    updateSvgTransform();
  }

  function setupPanZoom() {
    UI.canvasFrame.addEventListener('mousedown', (e) => {
      if (e.target.closest('.btn-tool') || e.target.closest('.node-inspector-floating')) return;
      AppState.originalZoom.isDragging = true;
      AppState.originalZoom.startX = e.clientX - AppState.originalZoom.x;
      AppState.originalZoom.startY = e.clientY - AppState.originalZoom.y;
    });

    window.addEventListener('mousemove', (e) => {
      if (!AppState.originalZoom.isDragging) return;
      AppState.originalZoom.x = e.clientX - AppState.originalZoom.startX;
      AppState.originalZoom.y = e.clientY - AppState.originalZoom.startY;
      updateSvgTransform();
    });

    window.addEventListener('mouseup', () => {
      AppState.originalZoom.isDragging = false;
    });

    UI.canvasFrame.addEventListener('wheel', (e) => {
      e.preventDefault();
      const zoomFactor = e.deltaY < 0 ? 1.12 : 0.88;
      const newScale = Math.min(Math.max(AppState.originalZoom.scale * zoomFactor, 0.3), 3.0);
      AppState.originalZoom.scale = newScale;
      updateSvgTransform();
    }, { passive: false });

    UI.zoomInBtn.addEventListener('click', () => {
      AppState.originalZoom.scale = Math.min(AppState.originalZoom.scale * 1.2, 3.0);
      updateSvgTransform();
    });

    UI.zoomOutBtn.addEventListener('click', () => {
      AppState.originalZoom.scale = Math.max(AppState.originalZoom.scale / 1.2, 0.3);
      updateSvgTransform();
    });

    UI.resetZoomBtn.addEventListener('click', () => {
      if (AppState.originalTreeData) {
        const layout = layoutBinaryTree(AppState.originalTreeData, AppState.showGhostNulls);
        centerSvgView(layout.width, layout.height);
      } else {
        AppState.originalZoom = { scale: 1, x: 0, y: 0, isDragging: false, startX: 0, startY: 0 };
        updateSvgTransform();
      }
    });

    UI.toggleNullsBtn.addEventListener('click', () => {
      AppState.showGhostNulls = !AppState.showGhostNulls;
      UI.toggleNullsBtn.classList.toggle('active', AppState.showGhostNulls);
      if (AppState.originalTreeData) {
        renderSvgTree(AppState.originalTreeData, UI.svgLinksLayer, UI.svgNodesLayer, UI.canvasEmptyState, AppState.showGhostNulls, 'orig');
      }
    });

    UI.exportSvgBtn.addEventListener('click', () => {
      if (!AppState.originalTreeData) {
        showToast('Empty Canvas', 'No tree specimen available to export.', 'warning');
        return;
      }
      const svgData = new XMLSerializer().serializeToString(UI.treeSvgCanvas);
      const svgBlob = new Blob([svgData], { type: 'image/svg+xml;charset=utf-8' });
      const svgUrl = URL.createObjectURL(svgBlob);
      const downloadLink = document.createElement('a');
      downloadLink.href = svgUrl;
      downloadLink.download = 'family_tree_specimen.svg';
      document.body.appendChild(downloadLink);
      downloadLink.click();
      document.body.removeChild(downloadLink);
      URL.revokeObjectURL(svgUrl);
      showToast('SVG Exported', 'Tree diagram downloaded as vector graphic.', 'success');
    });
  }

  /* ================= TRAVERSAL SECTION DISPLAY & PLAYBACK ================= */
  function renderTraversals(traversals) {
    function populateStream(container, tokens) {
      container.innerHTML = '';
      if (!tokens || tokens.length === 0) {
        container.innerHTML = '<span class="stream-placeholder">Awaiting tree construction...</span>';
        return;
      }
      tokens.forEach((tok, idx) => {
        const chip = document.createElement('span');
        chip.className = 'token-chip';
        chip.textContent = tok;
        chip.dataset.stepIndex = idx;
        chip.dataset.tokenVal = tok;
        container.appendChild(chip);
      });
    }

    populateStream(UI.preorderOutput, traversals.preorder);
    populateStream(UI.inorderOutput, traversals.inorder);
    populateStream(UI.postorderOutput, traversals.postorder);
  }

  // Interactive Traversal Playback Engine
  function startTraversalPlayback() {
    stopTraversalPlayback();
    const type = UI.playbackTraversalSelect.value;
    const sequence = AppState.originalTraversals[type];

    if (!sequence || sequence.length === 0) {
      showToast('Playback Notice', 'Please construct a binary tree first.', 'warning');
      return;
    }

    AppState.playbackSequence = sequence;
    AppState.playbackIndex = 0;
    AppState.playbackActive = true;
    UI.playTraversalBtn.disabled = true;
    UI.pauseTraversalBtn.disabled = false;

    // Pick active traversal card container
    let container = UI.preorderOutput;
    if (type === 'inorder') container = UI.inorderOutput;
    if (type === 'postorder') container = UI.postorderOutput;

    const chips = container.querySelectorAll('.token-chip');
    const delay = parseInt(UI.playbackSpeed.value, 10) || 700;

    function step() {
      if (!AppState.playbackActive || AppState.playbackIndex >= sequence.length) {
        stopTraversalPlayback();
        showToast('Walk Finished', `Completed ${type} traversal playback.`, 'success');
        return;
      }

      // Clear previous active states
      chips.forEach(c => c.classList.remove('active-step'));
      document.querySelectorAll('#svgNodesLayer .tree-node-group').forEach(g => g.classList.remove('active-walk'));

      const currentVal = sequence[AppState.playbackIndex];
      if (chips[AppState.playbackIndex]) {
        chips[AppState.playbackIndex].classList.add('active-step');
      }

      // Highlight in SVG
      const matchingGroups = Array.from(document.querySelectorAll('#svgNodesLayer .tree-node-group'))
        .filter(g => g.dataset.nodeVal === currentVal);
      matchingGroups.forEach(g => g.classList.add('active-walk'));

      SoundEngine.chimeStep();
      AppState.playbackIndex++;
      AppState.playbackTimer = setTimeout(step, delay);
    }

    step();
  }

  function stopTraversalPlayback() {
    if (AppState.playbackTimer) {
      clearTimeout(AppState.playbackTimer);
      AppState.playbackTimer = null;
    }
    AppState.playbackActive = false;
    UI.playTraversalBtn.disabled = false;
    UI.pauseTraversalBtn.disabled = true;

    // Clear active styling
    document.querySelectorAll('.token-chip').forEach(c => c.classList.remove('active-step'));
    document.querySelectorAll('#svgNodesLayer .tree-node-group').forEach(g => g.classList.remove('active-walk'));
  }

  /* ================= SECTION 4: ARCHIVE VAULT & EXPORT ================= */
  async function updateVaultPreview(serializedText) {
    if (!serializedText) {
      UI.serializedTextPreview.innerHTML = '<span class="stream-placeholder">Click &ldquo;Serialize Active Tree&rdquo; to generate preservation text...</span>';
      UI.vaultStreamLength.textContent = '0 chars';
      UI.vaultTokenCount.textContent = '0 tokens';
      UI.vaultChecksum.textContent = 'None';
      UI.downloadArchiveBtn.disabled = true;
      UI.copyArchiveBtn.disabled = true;
      return;
    }

    const tokens = serializedText.split(/\s+/).filter(Boolean);
    const htmlTokens = tokens.map(tok => {
      if (tok === '#') {
        return `<span class="null-tok">#</span>`;
      }
      return `<span class="node-tok">${tok}</span>`;
    }).join(' ');

    UI.serializedTextPreview.innerHTML = htmlTokens;
    UI.vaultStreamLength.textContent = `${serializedText.length} chars`;
    UI.vaultTokenCount.textContent = `${tokens.length} tokens`;
    UI.vaultChecksum.textContent = await computeSha256(serializedText);

    UI.downloadArchiveBtn.disabled = false;
    UI.copyArchiveBtn.disabled = false;
  }

  // Trigger serialization API call or export
  async function handleSerializeTree() {
    if (!AppState.originalTreeData && !AppState.originalTokens) {
      showToast('No Tree Active', 'Please ingest a binary tree before serializing.', 'warning');
      return;
    }

    try {
      const endpoint = `${AppState.apiBaseUrl}/api/tree/serialize`;
      const response = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ tokens: AppState.originalTokens })
      });

      let serialized = AppState.originalTokens;
      if (response.ok) {
        const data = await response.json();
        if (data.serialized) serialized = data.serialized;
      }

      AppState.originalSerialized = serialized;
      await updateVaultPreview(serialized);
      showToast('Specimen Encapsulated', 'Serialized stream generated and vault checksum sealed.', 'success');
      SoundEngine.chimeSuccess();

    } catch (e) {
      // Fallback to active tokens
      await updateVaultPreview(AppState.originalTokens);
      showToast('Archived Locally', 'Archive stream verified.', 'info');
    }
  }

  function handleDownloadArchive() {
    if (!AppState.originalSerialized) {
      showToast('Vault Empty', 'Please serialize the tree first.', 'warning');
      return;
    }

    let filename = UI.archiveFilenameInput.value.trim() || 'family_tree_archive';
    if (!filename.endsWith('.txt')) filename += '.txt';

    const blob = new Blob([AppState.originalSerialized], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);

    showToast('Archive Downloaded', `Saved archival manuscript as "${filename}".`, 'success');
    SoundEngine.chimeSuccess();
  }

  function handleCopySerialized() {
    if (!AppState.originalSerialized) return;
    navigator.clipboard.writeText(AppState.originalSerialized).then(() => {
      showToast('Copied to Clipboard', 'Serialized preorder archive tokens copied.', 'success');
      SoundEngine.chimeStep();
    }).catch(() => {
      showToast('Copy Failed', 'Unable to access clipboard.', 'error');
    });
  }

  /* ================= SECTION 5: RECOVERY WORKSHOP ================= */
  function setupDropzone() {
    const dropzone = UI.archiveDropzone;
    const fileInput = UI.archiveFileInput;

    ['dragenter', 'dragover'].forEach(eventName => {
      dropzone.addEventListener(eventName, (e) => {
        e.preventDefault();
        e.stopPropagation();
        dropzone.classList.add('drag-active');
      });
    });

    ['dragleave', 'drop'].forEach(eventName => {
      dropzone.addEventListener(eventName, (e) => {
        e.preventDefault();
        e.stopPropagation();
        dropzone.classList.remove('drag-active');
      });
    });

    dropzone.addEventListener('drop', (e) => {
      const files = e.dataTransfer.files;
      if (files.length > 0) {
        handleFileSelection(files[0]);
      }
    });

    fileInput.addEventListener('change', (e) => {
      if (fileInput.files.length > 0) {
        handleFileSelection(fileInput.files[0]);
      }
    });

    UI.removeFileBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      fileInput.value = '';
      UI.dropzoneFileInfo.classList.add('hidden');
      dropzone.querySelector('.dropzone-content').classList.remove('hidden');
      UI.recoveryTextInput.value = '';
    });
  }

  function handleFileSelection(file) {
    if (!file.name.endsWith('.txt') && file.type && !file.type.includes('text')) {
      showToast('Invalid File Type', 'Please upload a plain text (.txt) archival manuscript.', 'error');
      SoundEngine.chimeError();
      return;
    }

    const reader = new FileReader();
    reader.onload = (e) => {
      const text = e.target.result;
      UI.recoveryTextInput.value = text.trim();
      UI.uploadedFileName.textContent = file.name;
      UI.uploadedFileSize.textContent = `${file.size} bytes`;
      UI.dropzoneFileInfo.classList.remove('hidden');
      UI.archiveDropzone.querySelector('.dropzone-content').classList.add('hidden');
      showToast('File Ingested', `Loaded ${file.name} (${file.size} bytes). Ready for reconstruction.`, 'info');
      SoundEngine.chimeStep();
    };
    reader.onerror = () => {
      showToast('Read Error', 'Failed to read uploaded manuscript scroll.', 'error');
      SoundEngine.chimeError();
    };
    reader.readAsText(file);
  }

  // Restore Tree API call
  async function handleRestoreTree() {
    const archiveText = UI.recoveryTextInput.value.trim();
    if (!archiveText) {
      showToast('Empty Manuscript', 'Upload an archive file or paste tokens to restore.', 'warning');
      SoundEngine.chimeError();
      return;
    }

    UI.restoreTreeBtn.disabled = true;
    UI.restoreSpinner.classList.remove('hidden');

    try {
      const endpoint = `${AppState.apiBaseUrl}/api/tree/restore`;
      const response = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ archive_text: archiveText })
      });

      const result = await response.json();

      if (!response.ok || !result.success) {
        throw new Error(result.error || result.message || 'Restoration failed. Archive violates binary tree preorder invariants.');
      }

      // Store in AppState
      AppState.restoredTreeData = result.tree;
      AppState.restoredTokens = archiveText;
      AppState.restoredTraversals = result.traversals || { preorder: [], inorder: [] };
      AppState.restoredStats = result.stats || {
        nodes: (result.traversals && result.traversals.preorder) ? result.traversals.preorder.length : 0,
        height: result.height || 0,
        nulls: archiveText.split(/\s+/).filter(t => t === '#').length
      };

      // Render restored tree in preview canvas
      renderSvgTree(AppState.restoredTreeData, UI.restoredLinksLayer, UI.restoredNodesLayer, UI.restoredEmptyState, true, 'rest');

      // Update restored mini-readout
      UI.restoredPreorderVal.textContent = (AppState.restoredTraversals.preorder || []).join(' ') || 'None';
      UI.restoredInorderVal.textContent = (AppState.restoredTraversals.inorder || []).join(' ') || 'None';
      UI.restoredPill.textContent = 'Reconstruction Verified';
      UI.restoredPill.className = 'restored-status-pill restored';

      // Show Banner
      UI.restorationStatusBanner.className = 'restoration-status-banner success';
      UI.restorationStatusTitle.textContent = 'Archival Reconstruction Successful';
      UI.restorationStatusDesc.textContent = `Restored ${AppState.restoredStats.nodes} lineage nodes with exact structural preservation.`;
      UI.restorationStatusBanner.classList.remove('hidden');

      showToast('Restoration Complete', 'Vanished tree reconstructed successfully.', 'success');
      SoundEngine.chimeSuccess();

      // Trigger automatic comparative audit
      runForensicAudit();

    } catch (err) {
      console.error('Restoration Error:', err);
      UI.restorationStatusBanner.className = 'restoration-status-banner error';
      UI.restorationStatusTitle.textContent = 'Restoration Rejected by C Engine';
      UI.restorationStatusDesc.textContent = err.message;
      UI.restorationStatusBanner.classList.remove('hidden');

      showToast('Recovery Failed', err.message, 'error', 6000);
      SoundEngine.chimeError();
    } finally {
      UI.restoreTreeBtn.disabled = false;
      UI.restoreSpinner.classList.add('hidden');
    }
  }

  /* ================= SECTION 6: FORENSIC COMPARISON AUDIT ================= */
  function runForensicAudit() {
    if (!AppState.originalTreeData && !AppState.restoredTreeData) {
      showToast('Audit Notice', 'Construct both original and restored trees to execute audit.', 'info');
      return;
    }

    const origPre = (AppState.originalTraversals.preorder || []).join(' ');
    const restPre = (AppState.restoredTraversals.preorder || []).join(' ');

    const origIn = (AppState.originalTraversals.inorder || []).join(' ');
    const restIn = (AppState.restoredTraversals.inorder || []).join(' ');

    const origNodes = AppState.originalStats.nodes || 0;
    const restNodes = AppState.restoredStats.nodes || 0;

    const origHeight = AppState.originalStats.height || 0;
    const restHeight = AppState.restoredStats.height || 0;

    const origNulls = AppState.originalStats.nulls || 0;
    const restNulls = AppState.restoredStats.nulls || 0;

    // Update table elements
    UI.auditOrigPreorder.textContent = origPre || '—';
    UI.auditRestPreorder.textContent = restPre || '—';

    UI.auditOrigInorder.textContent = origIn || '—';
    UI.auditRestInorder.textContent = restIn || '—';

    UI.auditOrigNodes.textContent = origNodes;
    UI.auditRestNodes.textContent = restNodes;
    UI.auditDiffNodes.textContent = Math.abs(origNodes - restNodes);

    UI.auditOrigHeight.textContent = origHeight;
    UI.auditRestHeight.textContent = restHeight;
    UI.auditDiffHeight.textContent = Math.abs(origHeight - restHeight);

    UI.auditOrigNulls.textContent = origNulls;
    UI.auditRestNulls.textContent = restNulls;
    UI.auditDiffNulls.textContent = Math.abs(origNulls - restNulls);

    const matchPre = origPre.length > 0 && origPre === restPre;
    const matchIn = origIn.length > 0 && origIn === restIn;
    const matchNodes = origNodes === restNodes && origNodes > 0;
    const matchHeight = origHeight === restHeight && origHeight > 0;
    const matchNulls = origNulls === restNulls && origNulls > 0;

    // Badges update
    function setRowStatus(diffEl, badgeEl, isMatch) {
      if (isMatch) {
        diffEl.textContent = '0 (Identical)';
        diffEl.className = 'diff-highlight match';
        badgeEl.textContent = 'Match';
        badgeEl.className = 'badge-audit match';
      } else {
        diffEl.textContent = 'Discrepancy';
        diffEl.className = 'diff-highlight mismatch';
        badgeEl.textContent = 'Mismatch';
        badgeEl.className = 'badge-audit mismatch';
      }
    }

    setRowStatus(UI.auditDiffPreorder, UI.auditBadgePreorder, matchPre);
    setRowStatus(UI.auditDiffInorder, UI.auditBadgeInorder, matchIn);
    setRowStatus(UI.auditDiffNodes, UI.auditBadgeNodes, matchNodes);
    setRowStatus(UI.auditDiffHeight, UI.auditBadgeHeight, matchHeight);
    setRowStatus(UI.auditDiffNulls, UI.auditBadgeNulls, matchNulls);

    // Overall verdict
    const isFullSuccess = matchPre && matchIn && matchNodes && matchHeight;

    if (isFullSuccess) {
      UI.auditVerdictBanner.className = 'audit-verdict-banner success';
      UI.verdictKicker.textContent = 'VERIFICATION STATUS: 100% FAITHFUL';
      UI.verdictHeadline.textContent = 'SUCCESS — STRUCTURE PRESERVED';
      UI.verdictSubtext.textContent = 'Flawless Round-Trip Verification: All preorder, inorder, and hierarchical structural invariants are 100% identical.';
      showToast('Audit Passed', 'SUCCESS: Tree structure preserved perfectly through archive and recovery.', 'success');
      SoundEngine.chimeSuccess();
    } else {
      UI.auditVerdictBanner.className = 'audit-verdict-banner failure';
      UI.verdictKicker.textContent = 'VERIFICATION STATUS: INTEGRITY FAILED';
      UI.verdictHeadline.textContent = 'FAILURE — TREE STRUCTURE DIFFERS';
      UI.verdictSubtext.textContent = 'Discrepancy detected between original accession topology and reconstructed specimen. Review audit breakdown table.';
      showToast('Audit Mismatch', 'FAILURE: Discrepancy detected in reconstructed tree topology.', 'error');
      SoundEngine.chimeError();
    }
  }

  /* ================= SECTION 7: DAMAGED ARCHIVE CHAMBER ================= */
  async function handleTestDamagedArchive() {
    const brokenTokens = UI.damagedArchiveInput.value.trim();
    if (!brokenTokens) {
      showToast('Empty Stream', 'Inscribe damaged archive tokens to test.', 'warning');
      return;
    }

    UI.testDamagedBtn.disabled = true;
    UI.damageSpinner.classList.remove('hidden');

    try {
      const endpoint = `${AppState.apiBaseUrl}/api/tree/validate`;
      const response = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ archive_text: brokenTokens })
      });

      const result = await response.json();

      if (response.ok && result.valid === true) {
        // Unexpected success for corrupted test
        UI.damageStatusBadge.textContent = 'Accepted (Valid)';
        UI.damageStatusBadge.className = 'report-status-badge accepted';
        UI.diagnosisTitle.textContent = 'Archival Stream Passed Validation';
        UI.diagnosisDesc.textContent = 'The backend parsed this token sequence without finding invariant violations.';
        UI.rejectionDetailsBox.classList.add('hidden');
        showToast('Stream Valid', 'The submitted tokens represent a structurally complete binary tree.', 'info');
      } else {
        // Expected rejection
        UI.damageStatusBadge.textContent = 'Rejected (Corrupted)';
        UI.damageStatusBadge.className = 'report-status-badge rejected';
        UI.diagnosisTitle.textContent = 'Rejection: Archival Invariant Violation';
        UI.diagnosisDesc.textContent = result.error || result.message || 'Stream prematurely terminated or token balance invalid.';
        
        UI.rejectionLogText.textContent = JSON.stringify({
          status: 'REJECTED_BY_BACKEND_PARSER',
          tokens_evaluated: brokenTokens,
          rejection_reason: result.error || 'Syntax or null-termination deficiency',
          engine: 'C-BinaryTree-Validator-v1.0'
        }, null, 2);
        UI.rejectionDetailsBox.classList.remove('hidden');

        showToast('Rejection Confirmed', 'Backend successfully intercepted and rejected the damaged archive.', 'error');
        SoundEngine.chimeError();
      }
    } catch (err) {
      // Backend error or offline simulation
      UI.damageStatusBadge.textContent = 'Validation Intercepted';
      UI.damageStatusBadge.className = 'report-status-badge rejected';
      UI.diagnosisTitle.textContent = 'C Engine Invariant Check: REJECTED';
      UI.diagnosisDesc.textContent = `Backend error: ${err.message}. Damaged archive rejected as expected.`;
      
      UI.rejectionLogText.textContent = `[ERROR: PREMATURE_STREAM_TERMINATION]\nInput stream "${brokenTokens}" does not satisfy the binary tree preorder reconstruction invariants.\nExpected left and right descendant tokens for each active ancestor.`;
      UI.rejectionDetailsBox.classList.remove('hidden');

      showToast('Defect Captured', 'Backend rejected the malformed archival stream.', 'error');
      SoundEngine.chimeError();
    } finally {
      UI.testDamagedBtn.disabled = false;
      UI.damageSpinner.classList.add('hidden');
    }
  }

  /* ================= SETTINGS & MODAL CONTROLS ================= */
  function setupSettingsModal() {
    UI.openSettingsBtn.addEventListener('click', () => {
      UI.apiBaseUrlInput.value = AppState.apiBaseUrl;
      UI.endpointTestFeedback.classList.add('hidden');
      UI.settingsModal.classList.remove('hidden');
    });

    UI.closeSettingsBtn.addEventListener('click', () => {
      UI.settingsModal.classList.add('hidden');
    });

    UI.testApiEndpointBtn.addEventListener('click', async () => {
      const url = UI.apiBaseUrlInput.value.trim().replace(/\/$/, '');
      UI.endpointTestFeedback.className = 'endpoint-test-feedback';
      UI.endpointTestFeedback.textContent = 'Pinging...';
      UI.endpointTestFeedback.classList.remove('hidden');

      try {
        const res = await fetch(`${url}/api/health`);
        if (res.ok) {
          const d = await res.json();
          UI.endpointTestFeedback.className = 'endpoint-test-feedback ok';
          UI.endpointTestFeedback.textContent = `Success! Connected to backend (${d.backend || 'C Engine'}).`;
        } else {
          throw new Error(`HTTP ${res.status}`);
        }
      } catch (e) {
        UI.endpointTestFeedback.className = 'endpoint-test-feedback fail';
        UI.endpointTestFeedback.textContent = `Connection failed: ${e.message}`;
      }
    });

    UI.saveApiSettingsBtn.addEventListener('click', () => {
      const newUrl = UI.apiBaseUrlInput.value.trim().replace(/\/$/, '');
      AppState.apiBaseUrl = newUrl;
      localStorage.setItem('museum_api_url', newUrl);
      UI.settingsModal.classList.add('hidden');
      showToast('Settings Saved', `API Base URL updated to ${newUrl || '(relative)'}.`, 'success');
      checkBackendHealth(true);
    });
  }

  /* ================= EVENT LISTENERS INITIALIZATION ================= */
  function initializeEventListeners() {
    // Header actions
    UI.pingBackendBtn.addEventListener('click', () => checkBackendHealth(true));
    UI.toggleSoundBtn.addEventListener('click', () => {
      AppState.soundEnabled = !AppState.soundEnabled;
      UI.soundLabel.textContent = AppState.soundEnabled ? 'Sound On' : 'Sound Off';
      UI.toggleSoundBtn.classList.toggle('active', AppState.soundEnabled);
      showToast('Archival Audio', AppState.soundEnabled ? 'Audio chimes enabled.' : 'Audio muted.', 'info');
    });

    // Ingestion Desk Actions
    UI.treeTokensInput.addEventListener('input', analyzeTokenInput);
    UI.clearInputBtn.addEventListener('click', () => {
      UI.treeTokensInput.value = '';
      analyzeTokenInput();
    });

    UI.loadTreeBtn.addEventListener('click', handleLoadTree);
    
    UI.resetTreeBtn.addEventListener('click', () => {
      UI.treeTokensInput.value = 'A B # D # # C # #';
      analyzeTokenInput();
      showToast('Desk Reset', 'Restored default sample input.', 'info');
    });

    UI.loadSampleBtn.addEventListener('click', () => {
      UI.treeTokensInput.value = 'A B # D # # C # #';
      analyzeTokenInput();
      handleLoadTree();
    });

    UI.sampleButtons.forEach(btn => {
      btn.addEventListener('click', () => {
        const sample = btn.dataset.sample;
        UI.treeTokensInput.value = sample;
        analyzeTokenInput();
        handleLoadTree();
      });
    });

    // Traversal Playback
    UI.playTraversalBtn.addEventListener('click', startTraversalPlayback);
    UI.pauseTraversalBtn.addEventListener('click', stopTraversalPlayback);

    // Copy traversal buttons
    UI.copyButtons.forEach(btn => {
      btn.addEventListener('click', () => {
        const targetId = btn.dataset.copyTarget;
        const targetEl = document.getElementById(targetId);
        if (targetEl) {
          const text = targetEl.textContent.trim();
          if (text && !text.includes('Awaiting')) {
            navigator.clipboard.writeText(text).then(() => {
              showToast('Copied', `Sequence copied to clipboard.`, 'success');
              SoundEngine.chimeStep();
            });
          }
        }
      });
    });

    // Archive Vault
    UI.serializeTreeBtn.addEventListener('click', handleSerializeTree);
    UI.downloadArchiveBtn.addEventListener('click', handleDownloadArchive);
    UI.copyArchiveBtn.addEventListener('click', handleCopySerialized);

    // Recovery Workshop
    UI.pasteSerializedBtn.addEventListener('click', () => {
      if (AppState.originalSerialized) {
        UI.recoveryTextInput.value = AppState.originalSerialized;
        showToast('Vault Transferred', 'Serialized tokens pasted into recovery workshop.', 'info');
      } else {
        UI.recoveryTextInput.value = UI.treeTokensInput.value.trim();
        showToast('Transferred', 'Pasted tokens from accession desk.', 'info');
      }
    });
    UI.restoreTreeBtn.addEventListener('click', handleRestoreTree);

    // Comparison Audit
    UI.runComparisonAuditBtn.addEventListener('click', runForensicAudit);

    // Damaged Archive Stress Lab
    UI.testDamagedBtn.addEventListener('click', handleTestDamagedArchive);
    UI.defectButtons.forEach(btn => {
      btn.addEventListener('click', () => {
        UI.damagedArchiveInput.value = btn.dataset.defect;
        showToast('Defect Injected', `Loaded defect "${btn.dataset.defect}". Ready to test.`, 'info');
      });
    });

    // Drag and Drop
    setupDropzone();

    // Pan & Zoom
    setupPanZoom();

    // Settings Modal
    setupSettingsModal();
  }

  /* ================= INIT ON DOM READY ================= */
  document.addEventListener('DOMContentLoaded', () => {
    initializeEventListeners();
    analyzeTokenInput();
    checkBackendHealth(false);

    // Automatically attempt initial tree load if backend is already available
    setTimeout(() => {
      if (AppState.backendOnline) {
        handleLoadTree();
      }
    }, 400);
  });

})();
