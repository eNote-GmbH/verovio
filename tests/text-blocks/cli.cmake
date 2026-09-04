set(EXPORTED_MEI "${WORK_DIR}/text-blocks-cli.mei")
set(SVG_BASE "${WORK_DIR}/text-blocks-cli.svg")
file(REMOVE "${EXPORTED_MEI}")
file(GLOB OLD_SVG_PAGES "${WORK_DIR}/text-blocks-cli_*.svg")
if(OLD_SVG_PAGES)
    file(REMOVE ${OLD_SVG_PAGES})
endif()

execute_process(
    COMMAND "${VEROVIO}" -r "${RESOURCES}" -t mei -o "${EXPORTED_MEI}" "${FIXTURE}"
    RESULT_VARIABLE EXPORT_RESULT
)
if(NOT EXPORT_RESULT EQUAL 0 OR NOT EXISTS "${EXPORTED_MEI}")
    message(FATAL_ERROR "Verovio CLI did not export the text-block fixture")
endif()

file(READ "${EXPORTED_MEI}" EXPORTED_XML)
string(REGEX MATCHALL "tusk-syl-verse-2-line-[12]-[0-9][0-9][0-9]" EXTRACTED_SYLLABLES "${EXPORTED_XML}")
list(LENGTH EXTRACTED_SYLLABLES EXTRACTED_SYLLABLE_COUNT)
if(NOT EXTRACTED_SYLLABLE_COUNT EQUAL 92)
    message(FATAL_ERROR "CLI MEI export did not preserve exactly 92 extracted syllables")
endif()
if(NOT EXPORTED_XML MATCHES "<lg[^>]*xml:id=\"tusk-lg-verse-2\"")
    message(FATAL_ERROR "CLI MEI export did not preserve the later-verse line group")
endif()

# Rendering the exported file validates that the generated XML is parseable by
# the public CLI and produces usable SVG, rather than merely creating a file.
execute_process(
    COMMAND "${VEROVIO}" -r "${RESOURCES}" -a -o "${SVG_BASE}" "${EXPORTED_MEI}"
    RESULT_VARIABLE RENDER_RESULT
)
if(NOT RENDER_RESULT EQUAL 0)
    message(FATAL_ERROR "Verovio CLI could not re-import and render its MEI export")
endif()

file(GLOB SVG_PAGES "${WORK_DIR}/text-blocks-cli_*.svg")
list(SORT SVG_PAGES)
list(LENGTH SVG_PAGES SVG_PAGE_COUNT)
if(NOT SVG_PAGE_COUNT EQUAL 2)
    message(FATAL_ERROR "Expected the CLI fixture to render as exactly two SVG pages")
endif()

list(GET SVG_PAGES 0 FIRST_PAGE)
list(GET SVG_PAGES 1 SECOND_PAGE)
file(READ "${FIRST_PAGE}" FIRST_SVG)
file(READ "${SECOND_PAGE}" SECOND_SVG)
foreach(REQUIRED_ID tusk-div-later-verse-2 tusk-stack-verse-2-line-1-001)
    if(NOT FIRST_SVG MATCHES "${REQUIRED_ID}")
        message(FATAL_ERROR "The first fitting text fragment was missing from the score page: ${REQUIRED_ID}")
    endif()
endforeach()
foreach(REQUIRED_ID tusk-div-later-verse-2-continuation-1 tusk-stack-verse-2-line-2-046)
    if(NOT SECOND_SVG MATCHES "${REQUIRED_ID}")
        message(FATAL_ERROR "The overflowing text fragment was missing from the continuation page: ${REQUIRED_ID}")
    endif()
endforeach()

# The fixture has a 2100px page with 50px margins (Verovio's SVG viewBox uses
# ten units per px), so translated drawing origins must remain at or left of
# the 20500-unit right content boundary.
set(ALL_TEXT_SVG "${FIRST_SVG}${SECOND_SVG}")
string(REGEX MATCHALL "translate\\\(([0-9]+)" X_TRANSLATIONS "${ALL_TEXT_SVG}")
foreach(TRANSLATION ${X_TRANSLATIONS})
    string(REGEX REPLACE "translate\\\(" "" X "${TRANSLATION}")
    if(X GREATER 20500)
        message(FATAL_ERROR "CLI SVG content exceeded the fixture's right page boundary at x=${X}")
    endif()
endforeach()

# The default 2970px page has 50px top and bottom margins. Text-system
# translations therefore have to remain above the 29200-unit lower content
# boundary. This catches a visually overflowing final row.
string(REGEX MATCHALL "translate\\\([^, ]+[, ]+([0-9]+)" XY_TRANSLATIONS "${ALL_TEXT_SVG}")
foreach(TRANSLATION ${XY_TRANSLATIONS})
    string(REGEX REPLACE ".*[, ]+([0-9]+)$" "\\1" Y "${TRANSLATION}")
    if(Y GREATER 29200)
        message(FATAL_ERROR "CLI SVG content exceeded the page's lower content boundary at y=${Y}")
    endif()
endforeach()

set(TABLE_MEI "${WORK_DIR}/text-blocks-table-cli.mei")
set(TABLE_SVG "${WORK_DIR}/text-blocks-table-cli.svg")
file(REMOVE "${TABLE_MEI}")
file(GLOB OLD_TABLE_PAGES "${WORK_DIR}/text-blocks-table-cli_*.svg")
if(OLD_TABLE_PAGES)
    file(REMOVE ${OLD_TABLE_PAGES})
endif()
execute_process(
    COMMAND "${VEROVIO}" -r "${RESOURCES}" -t mei -o "${TABLE_MEI}" "${TABLE_FIXTURE}"
    RESULT_VARIABLE TABLE_EXPORT_RESULT
)
if(NOT TABLE_EXPORT_RESULT EQUAL 0 OR NOT EXISTS "${TABLE_MEI}")
    message(FATAL_ERROR "Verovio CLI did not export the table fixture")
endif()
file(READ "${TABLE_MEI}" TABLE_XML)
foreach(REQUIRED_XML "<table" "<caption" "<tr" "<td" "<th" "colspan=\"2\"" "rowspan=\"2\"")
    if(NOT TABLE_XML MATCHES "${REQUIRED_XML}")
        message(FATAL_ERROR "CLI MEI table export omitted ${REQUIRED_XML}")
    endif()
endforeach()
execute_process(
    COMMAND "${VEROVIO}" -r "${RESOURCES}" -a -o "${TABLE_SVG}" "${TABLE_MEI}"
    RESULT_VARIABLE TABLE_RENDER_RESULT
)
if(NOT TABLE_RENDER_RESULT EQUAL 0)
    message(FATAL_ERROR "Verovio CLI could not re-import and render its table export")
endif()
file(GLOB TABLE_PAGES "${WORK_DIR}/text-blocks-table-cli_*.svg")
if(EXISTS "${TABLE_SVG}")
    list(APPEND TABLE_PAGES "${TABLE_SVG}")
endif()
list(LENGTH TABLE_PAGES TABLE_PAGE_COUNT)
if(TABLE_PAGE_COUNT LESS 1)
    message(FATAL_ERROR "CLI table fixture produced no SVG pages")
endif()
set(TABLE_SVG_CONTENT "")
foreach(TABLE_PAGE ${TABLE_PAGES})
    file(READ "${TABLE_PAGE}" TABLE_PAGE_CONTENT)
    string(APPEND TABLE_SVG_CONTENT "${TABLE_PAGE_CONTENT}")
endforeach()
foreach(REQUIRED_ID two-column-table left-cell right-cell nested-table inline-table)
    if(NOT TABLE_SVG_CONTENT MATCHES "${REQUIRED_ID}")
        message(FATAL_ERROR "CLI table SVG omitted ${REQUIRED_ID}")
    endif()
endforeach()
# The CLI uses its configured 2100px page rather than scoreDef page dimensions;
# with 50px margins, the right content boundary is 20500 viewBox units.
string(REGEX MATCHALL "translate\\\(([0-9]+)" TABLE_X_TRANSLATIONS "${TABLE_SVG_CONTENT}")
foreach(TRANSLATION ${TABLE_X_TRANSLATIONS})
    string(REGEX REPLACE "translate\\\(" "" X "${TRANSLATION}")
    if(X GREATER 20500)
        message(FATAL_ERROR "CLI table SVG exceeded the fixture's right page boundary at x=${X}")
    endif()
endforeach()
