// chartStorage.js
export class ChartStorage {
    constructor() {
        this.dbName = 'ChartDatabase';
        this.storeName = 'chartData';
        this.version = 1;
        this.db = null;
    }

    /**
     * 打开数据库连接
     * @returns {Promise<IDBDatabase>} 数据库实例
     */
    open() {
        return new Promise((resolve, reject) => {
            const request = indexedDB.open(this.dbName, this.version);

            // 数据库版本升级 / 首次创建时触发
            request.onupgradeneeded = (event) => {
                this.db = event.target.result;
                // 若存储仓库不存在则创建（以 cardId 为唯一键）
                if (!this.db.objectStoreNames.contains(this.storeName)) {
                this.db.createObjectStore(this.storeName, { keyPath: 'cardId' });
                }
            };

            request.onsuccess = (event) => {
                this.db = event.target.result;
                resolve(this.db);
            };

            request.onerror = (event) => {
                console.error('IndexedDB 打开失败:', event.target.error);
                reject(event.target.error);
            };
        });
    }

    /**
     * 存储谱面 JSON 数据
     * @param {string|number} cardId 卡片唯一标识
     * @param {string} jsonStr 原始 JSON 字符串
     * @returns {Promise<boolean>} 存储成功返回 true
     */
    saveChart(cardId, jsonStr) {
        return new Promise((resolve, reject) => {
            if (!this.db) {
                reject(new Error('数据库未打开，请先调用 open()'));
                return;
            }
            // 开启读写事务
            const transaction = this.db.transaction([this.storeName], 'readwrite');
            const store = transaction.objectStore(this.storeName);
            // 存储数据（包含时间戳用于后续清理）
            const request = store.put({
                cardId,
                jsonStr,
                timestamp: Date.now()
            });

            request.onsuccess = () => resolve(true);
            request.onerror = () => reject(request.error);
        });
    }

    /**
     * 读取谱面 JSON 数据
     * @param {string|number} cardId 卡片唯一标识
     * @returns {Promise<string|null>} 原始 JSON 字符串（不存在则返回 null）
     */
    getChart(cardId) {
        return new Promise((resolve, reject) => {
            if (!this.db) {
                reject(new Error('数据库未打开，请先调用 open()'));
                return;
            }
            // 开启只读事务
            const transaction = this.db.transaction([this.storeName], 'readonly');
            const store = transaction.objectStore(this.storeName);
            const request = store.get(cardId);

            request.onsuccess = () => resolve(request.result?.jsonStr || null);
            request.onerror = () => reject(request.error);
        });
    }

    /**
     * 删除指定卡片的谱面数据
     * @param {string|number} cardId 卡片唯一标识
     * @returns {Promise<boolean>} 删除成功返回 true
     */
    deleteChart(cardId) {
        return new Promise((resolve, reject) => {
            if (!this.db) {
                reject(new Error('数据库未打开，请先调用 open()'));
                return;
            }
            const transaction = this.db.transaction([this.storeName], 'readwrite');
            const store = transaction.objectStore(this.storeName);
            const request = store.delete(cardId);

            request.onsuccess = () => resolve(true);
            request.onerror = () => reject(request.error);
        });
    }

    /**
     * 清空所有存储的谱面数据（可选方法，用于批量清理）
     * @returns {Promise<boolean>} 清空成功返回 true
     */
    clearAll() {
        return new Promise((resolve, reject) => {
            if (!this.db) {
                reject(new Error('数据库未打开，请先调用 open()'));
                return;
            }
            const transaction = this.db.transaction([this.storeName], 'readwrite');
            const store = transaction.objectStore(this.storeName);
            const request = store.clear();

            request.onsuccess = () => resolve(true);
            request.onerror = () => reject(request.error);
        });
    }
}