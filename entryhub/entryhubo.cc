#include "entryhubo.h"

#include <QJsonArray>

#include "global/entrypool.h"

EntryHubO::EntryHubO(CSectionInfo& info, QObject* parent)
    : EntryHub(info, parent)
{
}

/**
 * @brief Convert a JSON array of entries (from server) into an EntryList.
 *
 * This function iterates over the provided QJsonArray, creates Entry objects
 * allocated from the EntryPool according to the current section_, loads data
 * using Entry::ReadJson(), and finally returns them as an EntryList.
 *
 * **Design Notes**
 * 1. For the Order section:
 *    - The entries are managed by TableModelO.
 *
 * 2. For the Search function:
 *    - The entries are managed by SearchEntryModelO.
 *
 * 3. Entries fetched from the server are treated as **snapshot data**:
 *    - Once fetched, they are not affected by any external modifications.
 *    - Any changes can only be reflected by **fetching the data again from the server**.
 *
 * @param array The JSON array of entries retrieved from the server.
 * @return EntryList The constructed list of entries.
 */
EntryList EntryHubO::ProcessEntryArray(const QJsonArray& array)
{
    EntryList list {};

    for (const auto& value : array) {
        Q_ASSERT(value.isObject());

        const QJsonObject obj { value.toObject() };

        Entry* entry { EntryPool::Instance().Allocate(section_) };
        entry->ReadJson(obj);

        list.emplaceBack(entry);
    }

    return list;
}
