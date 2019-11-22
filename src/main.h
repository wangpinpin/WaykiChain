// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_MAIN_H
#define COIN_MAIN_H

#if defined(HAVE_CONFIG_H)
#include "config/coin-config.h"
#endif

#include <stdint.h>
#include <algorithm>
#include <exception>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "commons/arith_uint256.h"
#include "commons/types.h"
#include "commons/uint256.h"
#include "config/chainparams.h"
#include "config/const.h"
#include "config/errorcode.h"
#include "net.h"
#include "persistence/accountdb.h"
#include "persistence/block.h"
#include "persistence/blockdb.h"
#include "persistence/cachewrapper.h"
#include "persistence/delegatedb.h"
#include "persistence/dexdb.h"
#include "persistence/logdb.h"
#include "persistence/pricefeeddb.h"
#include "persistence/txdb.h"
#include "sigcache.h"
#include "tx/tx.h"
#include "tx/txserializer.h"
// #include "tx/accountregtx.h"
// #include "tx/cointransfertx.h"
// #include "tx/blockpricemediantx.h"
// #include "tx/blockrewardtx.h"
// #include "tx/cdptx.h"
// #include "tx/coinrewardtx.h"
// #include "tx/cointransfertx.h"
// #include "tx/contracttx.h"
// #include "tx/delegatetx.h"
// #include "tx/dextx.h"
// #include "tx/coinstaketx.h"
// #include "tx/mulsigtx.h"
// #include "tx/pricefeedtx.h"

// #include "tx/txmempool.h"
// #include "tx/assettx.h"
// #include "tx/wasmcontracttx.h"
// #include "tx/nickidregtx.h"

// class CBlockIndex;
class CBloomFilter;
class CChain;
class CInv;
// class CSysParamDBCache;
// class CBlockDBCache;
// class CAccountDBCache;

extern CCriticalSection cs_main;
/** The currently-connected chain of blocks. */
extern CChain chainActive;
extern CSignatureCache signatureCache;

extern CTxMemPool mempool;
extern map<uint256, CBlockIndex *> mapBlockIndex;
extern uint64_t nLastBlockTx;
extern uint64_t nLastBlockSize;
extern const string strMessageMagic;
extern bool ReadBlockFromDisk(const CBlockIndex *pIndex, CBlock &block) ;

extern bool mining;     // could be changed due to vote change
extern CKeyID minerKeyId;  // miner accout keyId
extern CKeyID nodeKeyId;   // first keyId of the node

static const string NetTypeNames[] = { "MAIN_NET", "TEST_NET", "REGTEST_NET" };

class CTxUndo;
class CValidationState;
class CWalletInterface;
class CTxMemCache;
class CUserCDP;

struct CNodeStateStats;

namespace {
struct CMainSignals {
    // Notifies listeners of updated transaction data (passing hash, transaction, and optionally the block it is found
    // in.
    boost::signals2::signal<void(const uint256 &, CBaseTx *, const CBlock *)> SyncTransaction;
    // Notifies listeners of an erased transaction (currently disabled, requires transaction replacement).
    boost::signals2::signal<void(const uint256 &)> EraseTransaction;
    // Notifies listeners of a new active block chain.
    boost::signals2::signal<void(const CBlockLocator &)> SetBestChain;
    // Notifies listeners about an inventory item being seen on the network.
    // boost::signals2::signal<void (const uint256 &)> Inventory;
    // Tells listeners to broadcast their data.
    boost::signals2::signal<void()> Broadcast;
} g_signals;
}  // namespace

/** Register a wallet to receive updates from core */
void RegisterWallet(CWalletInterface *pWalletIn);
/** Unregister a wallet from core */
void UnregisterWallet(CWalletInterface *pWalletIn);
/** Unregister all wallets from core */
void UnregisterAllWallets();
/** Push an updated transaction to all registered wallets */
void SyncTransaction(const uint256 &hash, CBaseTx *pBaseTx, const CBlock *pBlock = nullptr);
/** Erase Tx from wallets **/
void EraseTransaction(const uint256 &hash);
/** Register with a network node to receive its signals */
void RegisterNodeSignals(CNodeSignals &nodeSignals);
/** Unregister a network node */
void UnregisterNodeSignals(CNodeSignals &nodeSignals);
/** Check whether enough disk space is available for an incoming block */
bool CheckDiskSpace(uint64_t nAdditionalBytes = 0);

/** Verify consistency of the block and coin databases */
bool VerifyDB(int32_t nCheckLevel, int32_t nCheckDepth);

/** Run an instance of the script checking thread */
void ThreadScriptCheck();

/** Format a string that describes several potential problems detected by the core */
string GetWarnings(string strFor);
/** Retrieve a transaction (from memory pool, or from disk, if possible) */
bool GetTransaction(std::shared_ptr<CBaseTx> &pBaseTx, const uint256 &hash, CBlockDBCache &blockCache, bool bSearchMempool = true);
/** Retrieve a transaction height comfirmed in block*/
int32_t GetTxConfirmHeight(const uint256 &hash, CBlockDBCache &blockCache);

/** Abort with a message */
bool AbortNode(const string &msg);
/** Get statistics from node state */
bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats);
/** Increase a node's misbehavior score. */
void Misbehaving(NodeId nodeid, int32_t howmuch);

bool VerifySignature(const uint256 &sigHash, const std::vector<uint8_t> &signature, const CPubKey &pubKey);

/** (try to) add transaction to memory pool **/
bool AcceptToMemoryPool(CTxMemPool &pool, CValidationState &state, CBaseTx *pBaseTx,
                        bool fLimitFree, bool fRejectInsaneFee = false);

struct CNodeStateStats {
    int32_t nMisbehavior;
};

/** Check for standard transaction types
    @return True if all outputs (scriptPubKeys) use only standard transaction forms
*/
bool IsStandardTx(CBaseTx *pBaseTx, string &reason);

/** An in-memory indexed chain of blocks. */
class CChain {
private:
    vector<CBlockIndex *> vChain;
    CBlockIndex* finalityBlockIndex = nullptr ;

public:
    /** Returns the index entry for the genesis block of this chain, or nullptr if none. */
    CBlockIndex *Genesis() const {
        return vChain.size() > 0 ? vChain[0] : nullptr;
    }

    CBlockIndex *GetFinalityBlockIndex(){
        if(!finalityBlockIndex)
            return chainActive[0] ;
        return finalityBlockIndex ;
    }

    /** Returns the index entry for the tip of this chain, or nullptr if none. */
    CBlockIndex *Tip() const {
        return vChain.size() > 0 ? vChain[Height()] : nullptr;
    }

    /** Returns the index entry at a particular height in this chain, or nullptr if no such height exists. */
    CBlockIndex *operator[](int32_t height) const {
        if (height < 0 || height >= (int)vChain.size())
            return nullptr;
        return vChain[height];
    }

    /** Compare two chains efficiently. */
    friend bool operator==(const CChain &a, const CChain &b) {
        return a.vChain.size() == b.vChain.size() &&
               a.vChain[a.Height()] == b.vChain[b.Height()];
    }

    /** Efficiently check whether a block is present in this chain. */
    bool Contains(const CBlockIndex *pIndex) const {
        return (*this)[pIndex->height] == pIndex;
    }

    /** Find the successor of a block in this chain, or nullptr if the given index is not found or is the tip. */
    CBlockIndex *Next(const CBlockIndex *pIndex) const {
        if (Contains(pIndex))
            return (*this)[pIndex->height + 1];
        else
            return nullptr;
    }

    /** Return the maximal height in the chain. Is equal to chain.Tip() ? chain.Tip()->height : -1. */
    int32_t Height() const {
        return vChain.size() - 1;
    }


    bool UpdateFinalityBlock() ;
    /** Set/initialize a chain with a given tip. Returns the forking point. */
    CBlockIndex *SetTip(CBlockIndex *pIndex);

    /** Return a CBlockLocator that refers to a block in this chain (by default the tip). */
    CBlockLocator GetLocator(const CBlockIndex *pIndex = nullptr) const;

    /** Find the last common block between this chain and a locator. */
    CBlockIndex *FindFork(const CBlockLocator &locator) const;
}; //end of CChain

class CCacheDBManager {
public:
    CDBAccess           *pSysParamDb;
    CSysParamDBCache    *pSysParamCache;

    CDBAccess           *pAccountDb;
    CAccountDBCache     *pAccountCache;

    CDBAccess           *pAssetDb;
    CAssetDBCache       *pAssetCache;

    CDBAccess           *pContractDb;
    CContractDBCache    *pContractCache;

    CDBAccess           *pDelegateDb;
    CDelegateDBCache    *pDelegateCache;

    CDBAccess           *pCdpDb;
    CCdpDBCache         *pCdpCache;

    CDBAccess           *pClosedCdpDb;
    CClosedCdpDBCache   *pClosedCdpCache;

    CDBAccess           *pDexDb;
    CDexDBCache         *pDexCache;

    CBlockIndexDB       *pBlockIndexDb;

    CDBAccess           *pBlockDb;
    CBlockDBCache       *pBlockCache;

    CDBAccess           *pLogDb;
    CLogDBCache         *pLogCache;

    CDBAccess           *pReceiptDb;
    CTxReceiptDBCache   *pReceiptCache;

    CTxMemCache         *pTxCache;
    CPricePointMemCache *pPpCache;

public:
    CCacheDBManager(bool fReIndex, bool fMemory) {
        pSysParamDb     = new CDBAccess(DBNameType::SYSPARAM, false, fReIndex);
        pSysParamCache  = new CSysParamDBCache(pSysParamDb);

        pAccountDb      = new CDBAccess(DBNameType::ACCOUNT, false, fReIndex);
        pAccountCache   = new CAccountDBCache(pAccountDb);

        pAssetDb        = new CDBAccess(DBNameType::ASSET, false, fReIndex);
        pAssetCache     = new CAssetDBCache(pAssetDb);

        pContractDb     = new CDBAccess(DBNameType::CONTRACT, false, fReIndex);
        pContractCache  = new CContractDBCache(pContractDb);

        pDelegateDb     = new CDBAccess(DBNameType::DELEGATE, false, fReIndex);
        pDelegateCache  = new CDelegateDBCache(pDelegateDb);

        pCdpDb          = new CDBAccess(DBNameType::CDP, false, fReIndex);
        pCdpCache       = new CCdpDBCache(pCdpDb);

        pClosedCdpDb    = new CDBAccess(DBNameType::CLOSEDCDP, false, fReIndex);
        pClosedCdpCache = new CClosedCdpDBCache(pClosedCdpDb);

        pDexDb          = new CDBAccess(DBNameType::DEX, false, fReIndex);
        pDexCache       = new CDexDBCache(pDexDb);

        pBlockIndexDb   = new CBlockIndexDB(false, fReIndex);

        pBlockDb        = new CDBAccess(DBNameType::BLOCK, false, fReIndex);
        pBlockCache     = new CBlockDBCache(pBlockDb);

        pLogDb          = new CDBAccess(DBNameType::LOG, false, fReIndex);
        pLogCache       = new CLogDBCache(pLogDb);

        pReceiptDb      = new CDBAccess(DBNameType::RECEIPT, false, fReIndex);
        pReceiptCache   = new CTxReceiptDBCache(pReceiptDb);

        // memory-only cache
        pTxCache        = new CTxMemCache();
        pPpCache        = new CPricePointMemCache();
    }

    ~CCacheDBManager() {
        delete pSysParamCache;  pSysParamCache = nullptr;
        delete pAccountCache;   pAccountCache = nullptr;
        delete pAssetCache;     pAssetCache = nullptr;
        delete pContractCache;  pContractCache = nullptr;
        delete pDelegateCache;  pDelegateCache = nullptr;
        delete pCdpCache;       pCdpCache = nullptr;
        delete pClosedCdpCache; pClosedCdpCache = nullptr;
        delete pDexCache;       pDexCache = nullptr;
        delete pBlockCache;     pBlockCache = nullptr;
        delete pLogCache;       pLogCache = nullptr;
        delete pReceiptCache;   pReceiptCache = nullptr;

        delete pSysParamDb;     pSysParamDb = nullptr;
        delete pAccountDb;      pAccountDb = nullptr;
        delete pAssetDb;        pAssetDb = nullptr;
        delete pContractDb;     pContractDb = nullptr;
        delete pDelegateDb;     pDelegateDb = nullptr;
        delete pCdpDb;          pCdpDb = nullptr;
        delete pClosedCdpDb;    pClosedCdpDb = nullptr;
        delete pDexDb;          pDexDb = nullptr;
        delete pBlockIndexDb;   pBlockIndexDb = nullptr;
        delete pBlockDb;        pBlockDb = nullptr;
        delete pLogDb;          pLogDb = nullptr;
        delete pReceiptDb;      pReceiptDb = nullptr;

        // memory-only cache
        delete pTxCache;        pTxCache = nullptr;
        delete pPpCache;        pPpCache = nullptr;
    }

    bool Flush() {
        if (pSysParamCache) pSysParamCache->Flush();

        if (pAccountCache) pAccountCache->Flush();

        if (pAssetCache) pAssetCache->Flush();

        if (pContractCache) pContractCache->Flush();

        if (pDelegateCache) pDelegateCache->Flush();

        if (pCdpCache) pCdpCache->Flush();

        if (pClosedCdpCache) pClosedCdpCache->Flush();

        if (pDexCache) pDexCache->Flush();

        if (pBlockIndexDb) pBlockIndexDb->Flush();

        if (pBlockCache) pBlockCache->Flush();

        if (pLogCache) pLogCache->Flush();

        if (pReceiptCache) pReceiptCache->Flush();

        // Memory only cache, not bother to flush.
        // if (pTxCache)
        //     pTxCache->Flush();
        // if (pPpCache)
        //     pPpCache->Flush();

        return true;
    }
};  // CCacheDBManager

bool IsInitialBlockDownload();

/** Open an undo file (rev?????.dat) */
FILE *OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly = false);

/** Undo information for a CBlock */
class CBlockUndo {
public:
    vector<CTxUndo> vtxundo;

    IMPLEMENT_SERIALIZE(
        READWRITE(vtxundo);
    )

    bool WriteToDisk(CDiskBlockPos &pos, const uint256 &blockHash) {
        // Open history file to append
        CAutoFile fileout = CAutoFile(OpenUndoFile(pos), SER_DISK, CLIENT_VERSION);
        if (!fileout)
            return ERRORMSG("CBlockUndo::WriteToDisk : OpenUndoFile failed");

        // Write index header
        uint32_t nSize = fileout.GetSerializeSize(*this);
        fileout << FLATDATA(SysCfg().MessageStart()) << nSize;

        // Write undo data
        long fileOutPos = ftell(fileout);
        if (fileOutPos < 0)
            return ERRORMSG("CBlockUndo::WriteToDisk : ftell failed");
        pos.nPos = (uint32_t)fileOutPos;
        fileout << *this;

        // calculate & write checksum
        CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
        hasher << blockHash;
        hasher << *this;

        fileout << hasher.GetHash();

        // Flush stdio buffers and commit to disk before returning
        fflush(fileout);
        if (!IsInitialBlockDownload())
            FileCommit(fileout);

        return true;
    }

    bool ReadFromDisk(const CDiskBlockPos &pos, const uint256 &blockHash) {
        // Open history file to read
        CAutoFile filein = CAutoFile(OpenUndoFile(pos, true), SER_DISK, CLIENT_VERSION);
        if (!filein)
            return ERRORMSG("CBlockUndo::ReadFromDisk : OpenBlockFile failed");

        // Read block
        uint256 hashChecksum;
        try {
            filein >> *this;
            filein >> hashChecksum;
        } catch (std::exception &e) {
            return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
        }

        // Verify checksum
        CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
        hasher << blockHash;
        hasher << *this;

        if (hashChecksum != hasher.GetHash())
            return ERRORMSG("CBlockUndo::ReadFromDisk : Checksum mismatch");
        return true;
    }

    string ToString() const;
};


/** Data structure that represents a partial merkle tree.
 *
 * It respresents a subset of the txid's of a known block, in a way that
 * allows recovery of the list of txid's and the merkle root, in an
 * authenticated way.
 *
 * The encoding works as follows: we traverse the tree in depth-first order,
 * storing a bit for each traversed node, signifying whether the node is the
 * parent of at least one matched leaf txid (or a matched txid itself). In
 * case we are at the leaf level, or this bit is 0, its merkle node hash is
 * stored, and its children are not explorer further. Otherwise, no hash is
 * stored, but we recurse into both (or the only) child branch. During
 * decoding, the same depth-first traversal is performed, consuming bits and
 * hashes as they written during encoding.
 *
 * The serialization is fixed and provides a hard guarantee about the
 * encoded size:
 *
 *   SIZE <= 10 + ceil(32.25*N)
 *
 * Where N represents the number of leaf nodes of the partial tree. N itself
 * is bounded by:
 *
 *   N <= total_transactions
 *   N <= 1 + matched_transactions*tree_height
 *
 * The serialization format:
 *  - uint32     total_transactions (4 bytes)
 *  - varint     number of hashes   (1-3 bytes)
 *  - uint256[]  hashes in depth-first order (<= 32*N bytes)
 *  - varint     number of bytes of flag bits (1-3 bytes)
 *  - byte[]     flag bits, packed per 8 in a byte, least significant bit first (<= 2*N-1 bits)
 * The size constraints follow from this.
 */
class CPartialMerkleTree {
protected:
    // the total number of transactions in the block
    uint32_t nTransactions;

    // node-is-parent-of-matched-txid bits
    vector<bool> vBits;

    // txids and internal hashes
    vector<uint256> vHash;

    // flag set when encountering invalid data
    bool fBad;

    // helper function to efficiently calculate the number of nodes at given height in the merkle tree
    uint32_t CalcTreeWidth(int32_t height) {
        return (nTransactions + (1 << height) - 1) >> height;
    }

    // calculate the hash of a node in the merkle tree (at leaf level: the txid's themself)
    uint256 CalcHash(int32_t height, uint32_t pos, const vector<uint256> &vTxid);

    // recursive function that traverses tree nodes, storing the data as bits and hashes
    void TraverseAndBuild(int32_t height, uint32_t pos, const vector<uint256> &vTxid, const vector<bool> &vMatch);

    // recursive function that traverses tree nodes, consuming the bits and hashes produced by TraverseAndBuild.
    // it returns the hash of the respective node.
    uint256 TraverseAndExtract(int32_t height, uint32_t pos, uint32_t &nBitsUsed, uint32_t &nHashUsed, vector<uint256> &vMatch);

public:
    // serialization implementation
    IMPLEMENT_SERIALIZE(
        READWRITE(nTransactions);
        READWRITE(vHash);
        vector<uint8_t> vBytes;
        if (fRead) {
            READWRITE(vBytes);
            CPartialMerkleTree &us = *(const_cast<CPartialMerkleTree *>(this));
            us.vBits.resize(vBytes.size() * 8);
            for (uint32_t p = 0; p < us.vBits.size(); p++)
                us.vBits[p] = (vBytes[p / 8] & (1 << (p % 8))) != 0;
            us.fBad = false;
        } else {
            vBytes.resize((vBits.size() + 7) / 8);
            for (uint32_t p = 0; p < vBits.size(); p++)
                vBytes[p / 8] |= vBits[p] << (p % 8);
            READWRITE(vBytes);
        })

    // Construct a partial merkle tree from a list of transaction id's, and a mask that selects a subset of them
    CPartialMerkleTree(const vector<uint256> &vTxid, const vector<bool> &vMatch);

    CPartialMerkleTree();

    // extract the matching txid's represented by this partial merkle tree.
    // returns the merkle root, or 0 in case of failure
    uint256 ExtractMatches(vector<uint256> &vMatch);
};


/** Capture information about block/transaction validation */
class CValidationState {
private:
    enum mode_state {
        MODE_VALID,    // everything ok
        MODE_INVALID,  // network rule violation (DoS value may be set)
        MODE_ERROR,    // run-time error
    } mode;
    int32_t nDoS;
    string rejectReason;
    uint8_t rejectCode;
    bool corruptionPossible;

    string ret;

public:
    CValidationState() : mode(MODE_VALID), nDoS(0), corruptionPossible(false) {}
    bool DoS(int32_t level, bool ret = false, uint8_t rejectCodeIn = 0, string rejectReasonIn = "",
             bool corruptionIn = false) {
        rejectCode         = rejectCodeIn;
        rejectReason       = rejectReasonIn;
        corruptionPossible = corruptionIn;
        if (mode == MODE_ERROR)
            return ret;
        nDoS += level;
        mode = MODE_INVALID;
        return ret;
    }
    bool Invalid(bool ret = false, uint8_t rejectCodeIn = 0, string rejectReasonIn = "") {
        return DoS(0, ret, rejectCodeIn, rejectReasonIn);
    }
    bool Error(string rejectReasonIn = "") {
        if (mode == MODE_VALID)
            rejectReason = rejectReasonIn;
        mode = MODE_ERROR;
        return false;
    }
    bool Abort(const string &msg) {
        AbortNode(msg);
        return Error(msg);
    }
    bool IsValid() const { return mode == MODE_VALID; }
    bool IsInvalid() const { return mode == MODE_INVALID; }
    bool IsError() const { return mode == MODE_ERROR; }
    bool IsInvalid(int32_t &nDoSOut) const {
        if (IsInvalid()) {
            nDoSOut = nDoS;
            return true;
        }
        return false;
    }
    bool CorruptionPossible() const { return corruptionPossible; }
    uint8_t GetRejectCode() const { return rejectCode; }
    string GetRejectReason() const { return rejectReason; }

    void SetReturn(std::string r) {ret = r;}
    std::string GetReturn() {return ret;}
};

/** The currently best known chain of headers (some of which may be invalid). */
extern CChain chainMostWork;
extern CCacheDBManager *pCdMan;
extern int32_t nSyncTipHeight;
extern std::tuple<bool, boost::thread *> RunCoin(int32_t argc, char *argv[]);
extern string externalIp;

bool EraseBlockIndexFromSet(CBlockIndex *pIndex);

/** Used to relay blocks as header + vector<merkle branch>
 * to filtered nodes.
 */
class CMerkleBlock {
public:
    // Public only for unit testing
    CBlockHeader header;
    CPartialMerkleTree txn;

public:
    // Public only for unit testing and relay testing
    // (not relayed)
    vector<pair<uint32_t, uint256> > vMatchedTxn;

    // Create from a CBlock, filtering transactions according to filter
    // Note that this will call IsRelevantAndUpdate on the filter for each transaction,
    // thus the filter will likely be modified.
    CMerkleBlock(const CBlock &block, CBloomFilter &filter);

    IMPLEMENT_SERIALIZE(
        READWRITE(header);
        READWRITE(txn);)
};

class CWalletInterface {
protected:
    virtual void SyncTransaction(const uint256 &hash, CBaseTx *pBaseTx, const CBlock *pBlock) = 0;
    virtual void EraseTransaction(const uint256 &hash)                                        = 0;
    virtual void SetBestChain(const CBlockLocator &locator)                                   = 0;
    virtual void ResendWalletTransactions()                                                   = 0;
    friend void ::RegisterWallet(CWalletInterface *);
    friend void ::UnregisterWallet(CWalletInterface *);
    friend void ::UnregisterAllWallets();
};

/** Functions for validating blocks and updating the block tree */

/** Undo the effects of this block (with given index) on the UTXO set represented by coins.
 *  In case pfClean is provided, operation will try to be tolerant about errors, and *pfClean
 *  will be true if no problems were found. Otherwise, the return value will be false in case
 *  of problems. Note that in any case, coins may be modified. */
bool DisconnectBlock(CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex, CValidationState &state, bool *pfClean = nullptr);
// Apply the effects of this block (with given index) on the UTXO set represented by coins
bool ConnectBlock   (CBlock &block, CCacheWrapper &cw, CBlockIndex *pIndex, CValidationState &state, bool fJustCheck = false);

// Add this block to the block index, and if necessary, switch the active block chain to this
bool AddToBlockIndex(CBlock &block, CValidationState &state, const CDiskBlockPos &pos);

// Context-independent validity checks
bool CheckBlock(const CBlock &block, CValidationState &state, CCacheWrapper &cw,
                bool fCheckTx = true, bool fCheckMerkleRoot = true);

bool ProcessForkedChain(const CBlock &block, CValidationState &state);

// Store block on disk
// if dbp is provided, the file is known to already reside on disk
bool AcceptBlock(CBlock &block, CValidationState &state, CDiskBlockPos *dbp = nullptr);

//disconnect block for test
bool DisconnectBlockFromTip(CValidationState &state);

/** Mark a block as invalid. */
bool InvalidateBlock(CValidationState &state, CBlockIndex *pIndex);

/** Open a block file (blk?????.dat) */
FILE *OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly = false);

/** Import blocks from an external file */
bool LoadExternalBlockFile(FILE *fileIn, CDiskBlockPos *dbp = nullptr);
/** Initialize a new block tree database + block data on disk */
bool InitBlockIndex();
/** Load the block tree and coins database from disk */
bool LoadBlockIndex();
/** Unload database information */
void UnloadBlockIndex();
/** Push getblocks request */
void PushGetBlocks(CNode *pNode, CBlockIndex *pindexBegin, uint256 hashEnd);
/** Push getblocks request with different filtering strategies */
void PushGetBlocksOnCondition(CNode *pNode, CBlockIndex *pindexBegin, uint256 hashEnd);
/** Process an incoming block */
bool ProcessBlock(CValidationState &state, CNode *pFrom, CBlock *pBlock, CDiskBlockPos *dbp = nullptr);
/** Print the loaded block tree */
void PrintBlockTree();

void UpdateTime(CBlockHeader &block, const CBlockIndex *pIndexPrev);

/** Find the best known block, and make it the tip of the block chain */
bool ActivateBestChain(CValidationState &state);

/** Remove invalidity status from a block and its descendants. */
bool ReconsiderBlock(CValidationState &state, CBlockIndex *pIndex);
/** Functions for disk access for blocks */
bool WriteBlockToDisk(CBlock &block, CDiskBlockPos &pos);
bool ReadBlockFromDisk(const CDiskBlockPos &pos, CBlock &block);
bool ReadBlockFromDisk(const CBlockIndex *pIndex, CBlock &block);


bool ReadBaseTxFromDisk(const CTxCord txCord, std::shared_ptr<CBaseTx> &pTx);

template<typename TxType>
bool ReadTxFromDisk(const CTxCord txCord, std::shared_ptr<TxType> &pTx) {
    std::shared_ptr<CBaseTx> pBaseTx;
    if (!ReadBaseTxFromDisk(txCord, pBaseTx)) {
        return ERRORMSG("ReadTxFromDisk failed! txcord(%s)", txCord.ToString());
    }
    assert(pBaseTx);
    pTx = dynamic_pointer_cast<TxType>(pBaseTx);
    if (!pTx) {
        return ERRORMSG("The expected tx(%s) type is %s, but read tx type is %s",
            txCord.ToString(), typeid(TxType).name(), typeid(*pBaseTx).name());
    }
    return true;
}



#endif
